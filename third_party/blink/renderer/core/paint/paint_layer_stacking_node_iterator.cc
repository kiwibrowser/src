/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node_iterator.h"

// FIXME: We should build our primitive on top of
// PaintLayerStackingNode and remove this include.
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node.h"

namespace blink {

PaintLayerStackingNodeIterator::PaintLayerStackingNodeIterator(
    const PaintLayerStackingNode& root,
    unsigned which_children)
    : root_(root), remaining_children_(which_children), index_(0) {
  current_normal_flow_child_ = root.Layer()->FirstChild();
}

PaintLayerStackingNode* PaintLayerStackingNodeIterator::Next() {
  if (remaining_children_ & kNegativeZOrderChildren) {
    Vector<PaintLayerStackingNode*>* neg_z_order_list = root_.NegZOrderList();
    if (neg_z_order_list && index_ < neg_z_order_list->size())
      return neg_z_order_list->at(index_++);

    index_ = 0;
    remaining_children_ &= ~kNegativeZOrderChildren;
  }

  if (remaining_children_ & kNormalFlowChildren) {
    for (; current_normal_flow_child_;
         current_normal_flow_child_ =
             current_normal_flow_child_->NextSibling()) {
      if (!current_normal_flow_child_->StackingNode()->IsStacked()) {
        PaintLayer* normal_flow_child = current_normal_flow_child_;
        current_normal_flow_child_ = current_normal_flow_child_->NextSibling();
        return normal_flow_child->StackingNode();
      }
    }

    // We reset the iterator in case we reuse it.
    current_normal_flow_child_ = root_.Layer()->FirstChild();
    remaining_children_ &= ~kNormalFlowChildren;
  }

  if (remaining_children_ & kPositiveZOrderChildren) {
    Vector<PaintLayerStackingNode*>* pos_z_order_list = root_.PosZOrderList();
    if (pos_z_order_list && index_ < pos_z_order_list->size())
      return pos_z_order_list->at(index_++);

    index_ = 0;
    remaining_children_ &= ~kPositiveZOrderChildren;
  }

  return nullptr;
}

PaintLayerStackingNode* PaintLayerStackingNodeReverseIterator::Next() {
  if (remaining_children_ & kNegativeZOrderChildren) {
    Vector<PaintLayerStackingNode*>* neg_z_order_list = root_.NegZOrderList();
    if (neg_z_order_list && index_ >= 0)
      return neg_z_order_list->at(index_--);

    remaining_children_ &= ~kNegativeZOrderChildren;
    SetIndexToLastItem();
  }

  if (remaining_children_ & kNormalFlowChildren) {
    for (; current_normal_flow_child_;
         current_normal_flow_child_ =
             current_normal_flow_child_->PreviousSibling()) {
      if (!current_normal_flow_child_->StackingNode()->IsStacked()) {
        PaintLayer* normal_flow_child = current_normal_flow_child_;
        current_normal_flow_child_ =
            current_normal_flow_child_->PreviousSibling();
        return normal_flow_child->StackingNode();
      }
    }

    remaining_children_ &= ~kNormalFlowChildren;
    SetIndexToLastItem();
  }

  if (remaining_children_ & kPositiveZOrderChildren) {
    Vector<PaintLayerStackingNode*>* pos_z_order_list = root_.PosZOrderList();
    if (pos_z_order_list && index_ >= 0)
      return pos_z_order_list->at(index_--);

    remaining_children_ &= ~kPositiveZOrderChildren;
    SetIndexToLastItem();
  }

  return nullptr;
}

void PaintLayerStackingNodeReverseIterator::SetIndexToLastItem() {
  if (remaining_children_ & kNegativeZOrderChildren) {
    Vector<PaintLayerStackingNode*>* neg_z_order_list = root_.NegZOrderList();
    if (neg_z_order_list) {
      index_ = neg_z_order_list->size() - 1;
      return;
    }

    remaining_children_ &= ~kNegativeZOrderChildren;
  }

  if (remaining_children_ & kNormalFlowChildren) {
    current_normal_flow_child_ = root_.Layer()->LastChild();
    return;
  }

  if (remaining_children_ & kPositiveZOrderChildren) {
    Vector<PaintLayerStackingNode*>* pos_z_order_list = root_.PosZOrderList();
    if (pos_z_order_list) {
      index_ = pos_z_order_list->size() - 1;
      return;
    }

    remaining_children_ &= ~kPositiveZOrderChildren;
  }

  // No more list to visit.
  DCHECK(!remaining_children_);
  index_ = -1;
}

}  // namespace blink
