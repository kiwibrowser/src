/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/page/touch_disambiguation.h"

#include <algorithm>
#include <cmath>
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node_traversal.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_block.h"

namespace blink {

static IntRect BoundingBoxForEventNodes(Node* event_node) {
  if (!event_node->GetDocument().View())
    return IntRect();

  IntRect result;
  Node* node = event_node;
  while (node) {
    // Skip the whole sub-tree if the node doesn't propagate events.
    if (node != event_node && node->WillRespondToMouseClickEvents()) {
      node = NodeTraversal::NextSkippingChildren(*node, event_node);
      continue;
    }
    result.Unite(node->PixelSnappedBoundingBox());
    node = NodeTraversal::Next(*node, event_node);
  }
  return event_node->GetDocument().View()->ContentsToRootFrame(result);
}

static float ScoreTouchTarget(IntPoint touch_point,
                              int padding,
                              IntRect bounding_box) {
  if (bounding_box.IsEmpty())
    return 0;

  float reciprocal_padding = 1.f / padding;
  float score = 1;

  IntSize distance = bounding_box.DifferenceToPoint(touch_point);
  score *=
      std::max((padding - abs(distance.Width())) * reciprocal_padding, 0.f);
  score *=
      std::max((padding - abs(distance.Height())) * reciprocal_padding, 0.f);

  return score;
}

struct TouchTargetData {
  IntRect window_bounding_box;
  float score;
};

void FindGoodTouchTargets(const IntRect& touch_box_in_root_frame,
                          LocalFrame* main_frame,
                          Vector<IntRect>& good_targets,
                          HeapVector<Member<Node>>& highlight_nodes) {
  good_targets.clear();

  int touch_point_padding = ceil(std::max(touch_box_in_root_frame.Width(),
                                          touch_box_in_root_frame.Height()) *
                                 0.5);

  IntPoint touch_point = touch_box_in_root_frame.Center();
  IntPoint contents_point =
      main_frame->View()->RootFrameToContents(touch_point);

  HitTestResult result = main_frame->GetEventHandler().HitTestResultAtPoint(
      contents_point,
      HitTestRequest::kReadOnly | HitTestRequest::kActive |
          HitTestRequest::kListBased,
      LayoutRectOutsets(touch_point_padding, touch_point_padding,
                        touch_point_padding, touch_point_padding));
  const HeapListHashSet<Member<Node>>& hit_results =
      result.ListBasedTestResult();

  // Blacklist nodes that are container of disambiguated nodes.
  // It is not uncommon to have a clickable <div> that contains other clickable
  // objects.  This heuristic avoids excessive disambiguation in that case.
  HeapHashSet<Member<Node>> black_list;
  for (const auto& hit_result : hit_results) {
    // Ignore any Nodes that can't be clicked on.
    LayoutObject* layout_object = hit_result.Get()->GetLayoutObject();
    if (!layout_object || !hit_result.Get()->WillRespondToMouseClickEvents())
      continue;

    // Blacklist all of the Node's containers.
    for (LayoutBlock* container = layout_object->ContainingBlock(); container;
         container = container->ContainingBlock()) {
      Node* container_node = container->GetNode();
      if (!container_node)
        continue;
      if (!black_list.insert(container_node).is_new_entry)
        break;
    }
  }

  HeapHashMap<Member<Node>, TouchTargetData> touch_targets;
  float best_score = 0;
  for (const auto& hit_result : hit_results) {
    if (!hit_result)
      continue;
    for (Node& node : NodeTraversal::InclusiveAncestorsOf(*hit_result)) {
      if (black_list.Contains(&node))
        continue;
      if (node.IsDocumentNode() || IsHTMLHtmlElement(node) ||
          IsHTMLBodyElement(node))
        break;
      if (node.WillRespondToMouseClickEvents()) {
        TouchTargetData& target_data =
            touch_targets.insert(&node, TouchTargetData()).stored_value->value;
        target_data.window_bounding_box = BoundingBoxForEventNodes(&node);
        target_data.score = ScoreTouchTarget(touch_point, touch_point_padding,
                                             target_data.window_bounding_box);
        best_score = std::max(best_score, target_data.score);
        break;
      }
    }
  }

  // The scoring function uses the overlap area with the fat point as the score.
  // We ignore the candidates that have less than this (empirically tuned)
  // fraction of overlap than the best candidate to avoid excessive popups.
  //
  // If this value were 1, then the disambiguation feature would only be seen
  // when two nodes have precisely the same overlap with the touch radius.  If
  // it were 0, then any miniscule overlap with the edge of another node would
  // trigger it.
  const float kRelativeAmbiguityThreshold = 0.75f;

  for (const auto& touch_target : touch_targets) {
    if (touch_target.value.score < best_score * kRelativeAmbiguityThreshold)
      continue;
    good_targets.push_back(touch_target.value.window_bounding_box);
    highlight_nodes.push_back(touch_target.key);
  }
}

}  // namespace blink
