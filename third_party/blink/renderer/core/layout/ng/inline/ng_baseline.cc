// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_baseline.h"

#include "third_party/blink/renderer/core/layout/layout_block.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_node.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_input_node.h"

namespace blink {

bool operator==(const NGBaselineRequest& lhs, const NGBaselineRequest& rhs) {
  return lhs.algorithm_type == rhs.algorithm_type &&
         lhs.baseline_type == rhs.baseline_type;
}

bool operator!=(const NGBaselineRequest& lhs, const NGBaselineRequest& rhs) {
  return !(lhs == rhs);
}

bool NGBaseline::ShouldPropagateBaselines(const NGLayoutInputNode node) {
  if (node.IsInline())
    return true;

  return ShouldPropagateBaselines(ToLayoutBox(node.GetLayoutObject()));
}

bool NGBaseline::ShouldPropagateBaselines(LayoutBox* layout_box) {
  // Test if this node should use its own box to synthesize the baseline.
  if (!layout_box->IsLayoutBlock() ||
      layout_box->IsFloatingOrOutOfFlowPositioned() ||
      layout_box->IsWritingModeRoot())
    return false;

  // If this node is LayoutBlock that uses old layout, this may be a subclass
  // that overrides baseline functions. Propagate baseline requests so that we
  // call virtual functions.
  if (!NGBlockNode(layout_box).CanUseNewLayout())
    return true;

  // CSS defines certain cases to synthesize baselines from box. See comments in
  // UseLogicalBottomMarginEdgeForInlineBlockBaseline().
  const LayoutBlock* layout_block = ToLayoutBlock(layout_box);
  if (layout_block->UseLogicalBottomMarginEdgeForInlineBlockBaseline())
    return false;

  return true;
}

}  // namespace blink
