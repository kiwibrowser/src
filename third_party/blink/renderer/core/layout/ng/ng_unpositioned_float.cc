// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_unpositioned_float.h"

#include "third_party/blink/renderer/core/layout/ng/ng_block_break_token.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

// Define the constructor and destructor here, so that we can forward-declare
// more in the header file.
NGUnpositionedFloat::NGUnpositionedFloat(const NGBoxStrut& margins,
                                         const NGLogicalSize& available_size,
                                         const NGLogicalSize& percentage_size,
                                         LayoutUnit origin_bfc_line_offset,
                                         LayoutUnit bfc_line_offset,
                                         NGBlockNode node,
                                         NGBlockBreakToken* token)
    : node(node),
      token(token),
      available_size(available_size),
      percentage_size(percentage_size),
      origin_bfc_line_offset(origin_bfc_line_offset),
      bfc_line_offset(bfc_line_offset),
      margins(margins) {}

NGUnpositionedFloat::~NGUnpositionedFloat() = default;

bool NGUnpositionedFloat::IsLeft() const {
  return node.Style().Floating() == EFloat::kLeft;
}

bool NGUnpositionedFloat::IsRight() const {
  return node.Style().Floating() == EFloat::kRight;
}

EClear NGUnpositionedFloat::ClearType() const {
  return node.Style().Clear();
}

}  // namespace blink
