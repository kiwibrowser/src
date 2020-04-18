/*
 * Copyright (C) 2003, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_STACKING_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_STACKING_NODE_H_

#include <memory>
#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class PaintLayer;
class PaintLayerCompositor;
class ComputedStyle;
class LayoutBoxModelObject;

// PaintLayerStackingNode represents a stacked element which is either a
// stacking context or a positioned element.
// See
// https://chromium.googlesource.com/chromium/src.git/+/master/third_party/blink/renderer/core/paint/README.md
// for more details of stacked elements.
//
// Stacked elements are the basis for the CSS painting algorithm. The paint
// order is determined by walking stacked elements in an order defined by
// ‘z-index’. This walk is interleaved with non-stacked contents.
// See CSS 2.1 appendix E for the actual algorithm
// http://www.w3.org/TR/CSS21/zindex.html
// See also PaintLayerPainter (in particular paintLayerContents) for
// our implementation of the walk.
//
// Stacked elements form a subtree over the layout tree. Ideally we would want
// objects of this class to be a node in this tree but there are potential
// issues with stale pointers so we rely on PaintLayer's tree
// structure.
//
// This class's purpose is to represent a node in the stacked element tree
// (aka paint tree). It currently caches the z-order lists for painting and
// hit-testing.
//
// To implement any z-order list iterations, use
// PaintLayerStackingNodeIterator and
// PaintLayerStackingNodeReverseIterator.
//
// Only a real stacking context can have non-empty z-order lists thus contain
// child nodes in the tree. The z-order lists of a positioned element with auto
// z-index are always empty (i.e. it's a leaf of the stacked element tree).
// A real stacking context can also be a leaf if it doesn't contain any stacked
// elements.
class CORE_EXPORT PaintLayerStackingNode {
  USING_FAST_MALLOC(PaintLayerStackingNode);

 public:
  explicit PaintLayerStackingNode(PaintLayer*);
  ~PaintLayerStackingNode();

  int ZIndex() const { return GetLayoutObject().Style()->ZIndex(); }

  bool IsStackingContext() const {
    return GetLayoutObject().Style()->IsStackingContext();
  }

  // Whether the node is stacked. See documentation for the class about
  // "stacked".  For now every PaintLayer has a PaintLayerStackingNode, even if
  // the layer is not stacked (e.g. a scrollable layer which is statically
  // positioned and is not a stacking context).
  bool IsStacked() const { return is_stacked_; }

  // Update our normal and z-index lists.
  void UpdateLayerListsIfNeeded();

  bool ZOrderListsDirty() const { return z_order_lists_dirty_; }
  void DirtyZOrderLists();
  void UpdateZOrderLists();
  void ClearZOrderLists();
  void DirtyStackingContextZOrderLists();

  bool HasPositiveZOrderList() const {
    return PosZOrderList() && PosZOrderList()->size();
  }
  bool HasNegativeZOrderList() const {
    return NegZOrderList() && NegZOrderList()->size();
  }

  void StyleDidChange(const ComputedStyle* old_style);

  PaintLayerStackingNode* AncestorStackingContextNode() const;

  PaintLayer* Layer() const { return layer_; }

#if DCHECK_IS_ON()
  bool LayerListMutationAllowed() const { return layer_list_mutation_allowed_; }
  void SetLayerListMutationAllowed(bool flag) {
    layer_list_mutation_allowed_ = flag;
  }
#endif

 private:
  friend class PaintLayerStackingNodeIterator;
  friend class PaintLayerStackingNodeReverseIterator;
  friend class LayoutTreeAsText;

  Vector<PaintLayerStackingNode*>* PosZOrderList() const {
    DCHECK(!z_order_lists_dirty_);
    DCHECK(IsStackingContext() || !pos_z_order_list_);
    return pos_z_order_list_.get();
  }

  Vector<PaintLayerStackingNode*>* NegZOrderList() const {
    DCHECK(!z_order_lists_dirty_);
    DCHECK(IsStackingContext() || !neg_z_order_list_);
    return neg_z_order_list_.get();
  }

  void RebuildZOrderLists();
  void CollectLayers(
      std::unique_ptr<Vector<PaintLayerStackingNode*>>& pos_z_order_list,
      std::unique_ptr<Vector<PaintLayerStackingNode*>>& neg_z_order_list);

#if DCHECK_IS_ON()
  bool IsInStackingParentZOrderLists() const;
  void UpdateStackingParentForZOrderLists(
      PaintLayerStackingNode* stacking_parent);
  void SetStackingParent(PaintLayerStackingNode* stacking_parent) {
    stacking_parent_ = stacking_parent;
  }
#endif

  bool IsDirtyStackingContext() const {
    return z_order_lists_dirty_ && IsStackingContext();
  }

  PaintLayerCompositor* Compositor() const;
  // We can't return a LayoutBox as LayoutInline can be a stacking context.
  LayoutBoxModelObject& GetLayoutObject() const;

  PaintLayer* layer_;

  // m_posZOrderList holds a sorted list of all the descendant nodes within
  // that have z-indices of 0 (or is treated as 0 for positioned objects) or
  // greater.
  std::unique_ptr<Vector<PaintLayerStackingNode*>> pos_z_order_list_;
  // m_negZOrderList holds descendants within our stacking context with
  // negative z-indices.
  std::unique_ptr<Vector<PaintLayerStackingNode*>> neg_z_order_list_;

  // This boolean caches whether the z-order lists above are dirty.
  // It is only ever set for stacking contexts, as no other element can
  // have z-order lists.
  bool z_order_lists_dirty_ : 1;

  // This attribute caches whether the element was stacked. It's needed to check
  // the current stacked status (instead of the new stacked status determined by
  // the new style which has not been realized yet) when a layer is removed due
  // to style change.
  bool is_stacked_ : 1;

#if DCHECK_IS_ON()
  bool layer_list_mutation_allowed_ : 1;
  PaintLayerStackingNode* stacking_parent_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PaintLayerStackingNode);
};

inline void PaintLayerStackingNode::ClearZOrderLists() {
  DCHECK(!IsStackingContext());

#if DCHECK_IS_ON()
  UpdateStackingParentForZOrderLists(nullptr);
#endif

  pos_z_order_list_.reset();
  neg_z_order_list_.reset();
}

inline void PaintLayerStackingNode::UpdateZOrderLists() {
  if (!z_order_lists_dirty_)
    return;

  if (!IsStackingContext()) {
    ClearZOrderLists();
    z_order_lists_dirty_ = false;
    return;
  }

  RebuildZOrderLists();
}

#if DCHECK_IS_ON()
class LayerListMutationDetector {
 public:
  explicit LayerListMutationDetector(PaintLayerStackingNode* stacking_node)
      : stacking_node_(stacking_node),
        previous_mutation_allowed_state_(
            stacking_node->LayerListMutationAllowed()) {
    stacking_node_->SetLayerListMutationAllowed(false);
  }

  ~LayerListMutationDetector() {
    stacking_node_->SetLayerListMutationAllowed(
        previous_mutation_allowed_state_);
  }

 private:
  PaintLayerStackingNode* stacking_node_;
  bool previous_mutation_allowed_state_;
};
#endif

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_STACKING_NODE_H_
