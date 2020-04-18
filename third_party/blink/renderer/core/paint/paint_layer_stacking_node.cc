/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights
 * reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Robert O'Callahan <roc+@cs.cmu.edu>
 *   David Baron <dbaron@fas.harvard.edu>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Randall Jesup <rjesup@wgate.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Josh Soref <timeless@mac.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node.h"

#include <algorithm>
#include <memory>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/layout/layout_multi_column_flow_thread.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

// FIXME: This should not require PaintLayer. There is currently a cycle where
// in order to determine if we isStacked() we have to ask the paint
// layer about some of its state.
PaintLayerStackingNode::PaintLayerStackingNode(PaintLayer* layer)
    : layer_(layer)
#if DCHECK_IS_ON()
      ,
      layer_list_mutation_allowed_(true),
      stacking_parent_(nullptr)
#endif
{
  is_stacked_ = GetLayoutObject().StyleRef().IsStacked();

  // Non-stacking contexts should have empty z-order lists. As this is already
  // the case, there is no need to dirty / recompute these lists.
  z_order_lists_dirty_ = IsStackingContext();
}

PaintLayerStackingNode::~PaintLayerStackingNode() {
#if DCHECK_IS_ON()
  if (!GetLayoutObject().DocumentBeingDestroyed()) {
    DCHECK(!IsInStackingParentZOrderLists());

    UpdateStackingParentForZOrderLists(nullptr);
  }
#endif
}

// Helper for the sorting of layers by z-index.
static inline bool CompareZIndex(PaintLayerStackingNode* first,
                                 PaintLayerStackingNode* second) {
  return first->ZIndex() < second->ZIndex();
}

PaintLayerCompositor* PaintLayerStackingNode::Compositor() const {
  DCHECK(GetLayoutObject().View());
  if (!GetLayoutObject().View())
    return nullptr;
  return GetLayoutObject().View()->Compositor();
}

void PaintLayerStackingNode::DirtyZOrderLists() {
#if DCHECK_IS_ON()
  DCHECK(layer_list_mutation_allowed_);
#endif
  DCHECK(IsStackingContext());

#if DCHECK_IS_ON()
  UpdateStackingParentForZOrderLists(nullptr);
#endif

  if (pos_z_order_list_)
    pos_z_order_list_->clear();
  if (neg_z_order_list_)
    neg_z_order_list_->clear();
  z_order_lists_dirty_ = true;

  if (!GetLayoutObject().DocumentBeingDestroyed() && Compositor())
    Compositor()->SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
}

void PaintLayerStackingNode::DirtyStackingContextZOrderLists() {
  if (PaintLayerStackingNode* stacking_node = AncestorStackingContextNode())
    stacking_node->DirtyZOrderLists();
}

void PaintLayerStackingNode::RebuildZOrderLists() {
#if DCHECK_IS_ON()
  DCHECK(layer_list_mutation_allowed_);
#endif
  DCHECK(IsDirtyStackingContext());

  for (PaintLayer* child = Layer()->FirstChild(); child;
       child = child->NextSibling())
    child->StackingNode()->CollectLayers(pos_z_order_list_, neg_z_order_list_);

  // Sort the two lists.
  if (pos_z_order_list_)
    std::stable_sort(pos_z_order_list_->begin(), pos_z_order_list_->end(),
                     CompareZIndex);

  if (neg_z_order_list_)
    std::stable_sort(neg_z_order_list_->begin(), neg_z_order_list_->end(),
                     CompareZIndex);

  // Append layers for top layer elements after normal layer collection, to
  // ensure they are on top regardless of z-indexes.  The layoutObjects of top
  // layer elements are children of the view, sorted in top layer stacking
  // order.
  if (Layer()->IsRootLayer()) {
    LayoutBlockFlow* root_block = GetLayoutObject().View();
    // If the viewport is paginated, everything (including "top-layer" elements)
    // gets redirected to the flow thread. So that's where we have to look, in
    // that case.
    if (LayoutBlockFlow* multi_column_flow_thread =
            root_block->MultiColumnFlowThread())
      root_block = multi_column_flow_thread;
    for (LayoutObject* child = root_block->FirstChild(); child;
         child = child->NextSibling()) {
      Element* child_element =
          (child->GetNode() && child->GetNode()->IsElementNode())
              ? ToElement(child->GetNode())
              : nullptr;
      if (child_element && child_element->IsInTopLayer()) {
        PaintLayer* layer = ToLayoutBoxModelObject(child)->Layer();
        // Create the buffer if it doesn't exist yet.
        if (!pos_z_order_list_) {
          pos_z_order_list_ =
              std::make_unique<Vector<PaintLayerStackingNode*>>();
        }
        pos_z_order_list_->push_back(layer->StackingNode());
      }
    }
  }

#if DCHECK_IS_ON()
  UpdateStackingParentForZOrderLists(this);
#endif

  z_order_lists_dirty_ = false;
}

void PaintLayerStackingNode::CollectLayers(
    std::unique_ptr<Vector<PaintLayerStackingNode*>>& pos_buffer,
    std::unique_ptr<Vector<PaintLayerStackingNode*>>& neg_buffer) {
  if (Layer()->IsInTopLayer())
    return;

  if (IsStacked()) {
    std::unique_ptr<Vector<PaintLayerStackingNode*>>& buffer =
        (ZIndex() >= 0) ? pos_buffer : neg_buffer;
    if (!buffer)
      buffer = std::make_unique<Vector<PaintLayerStackingNode*>>();
    buffer->push_back(this);
  }

  if (!IsStackingContext()) {
    for (PaintLayer* child = Layer()->FirstChild(); child;
         child = child->NextSibling())
      child->StackingNode()->CollectLayers(pos_buffer, neg_buffer);
  }
}

#if DCHECK_IS_ON()
bool PaintLayerStackingNode::IsInStackingParentZOrderLists() const {
  if (!stacking_parent_ || stacking_parent_->ZOrderListsDirty())
    return false;

  if (stacking_parent_->PosZOrderList() &&
      stacking_parent_->PosZOrderList()->Find(this) != kNotFound)
    return true;

  if (stacking_parent_->NegZOrderList() &&
      stacking_parent_->NegZOrderList()->Find(this) != kNotFound)
    return true;

  return false;
}

void PaintLayerStackingNode::UpdateStackingParentForZOrderLists(
    PaintLayerStackingNode* stacking_parent) {
  if (pos_z_order_list_) {
    for (size_t i = 0; i < pos_z_order_list_->size(); ++i)
      pos_z_order_list_->at(i)->SetStackingParent(stacking_parent);
  }

  if (neg_z_order_list_) {
    for (size_t i = 0; i < neg_z_order_list_->size(); ++i)
      neg_z_order_list_->at(i)->SetStackingParent(stacking_parent);
  }
}

#endif

void PaintLayerStackingNode::UpdateLayerListsIfNeeded() {
  UpdateZOrderLists();
}

void PaintLayerStackingNode::StyleDidChange(const ComputedStyle* old_style) {
  bool was_stacking_context =
      old_style ? old_style->IsStackingContext() : false;
  int old_z_index = old_style ? old_style->ZIndex() : 0;

  bool is_stacking_context = IsStackingContext();
  bool should_be_stacked = GetLayoutObject().StyleRef().IsStacked();
  if (is_stacking_context == was_stacking_context &&
      is_stacked_ == should_be_stacked && old_z_index == ZIndex())
    return;

  DirtyStackingContextZOrderLists();

  if (is_stacking_context)
    DirtyZOrderLists();
  else
    ClearZOrderLists();

  if (is_stacked_ != should_be_stacked) {
    is_stacked_ = should_be_stacked;
    if (!GetLayoutObject().DocumentBeingDestroyed() &&
        !Layer()->IsRootLayer() && Compositor())
      Compositor()->SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
  }
}

PaintLayerStackingNode* PaintLayerStackingNode::AncestorStackingContextNode()
    const {
  for (PaintLayer* ancestor = Layer()->Parent(); ancestor;
       ancestor = ancestor->Parent()) {
    PaintLayerStackingNode* stacking_node = ancestor->StackingNode();
    if (stacking_node->IsStackingContext())
      return stacking_node;
  }
  return nullptr;
}

LayoutBoxModelObject& PaintLayerStackingNode::GetLayoutObject() const {
  return layer_->GetLayoutObject();
}

}  // namespace blink
