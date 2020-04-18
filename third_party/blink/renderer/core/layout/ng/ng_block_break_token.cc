// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_block_break_token.h"

#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

NGBlockBreakToken::NGBlockBreakToken(
    NGLayoutInputNode node,
    LayoutUnit used_block_size,
    Vector<scoped_refptr<NGBreakToken>>& child_break_tokens,
    bool has_last_resort_break)
    : NGBreakToken(kBlockBreakToken, kUnfinished, node),
      used_block_size_(used_block_size),
      has_last_resort_break_(has_last_resort_break) {
  child_break_tokens_.swap(child_break_tokens);
}

NGBlockBreakToken::NGBlockBreakToken(NGLayoutInputNode node,
                                     LayoutUnit used_block_size,
                                     bool has_last_resort_break)
    : NGBreakToken(kBlockBreakToken, kFinished, node),
      used_block_size_(used_block_size),
      has_last_resort_break_(has_last_resort_break) {}

NGBlockBreakToken::NGBlockBreakToken(NGLayoutInputNode node)
    : NGBreakToken(kBlockBreakToken, kUnfinished, node) {}

#ifndef NDEBUG

String NGBlockBreakToken::ToString() const {
  StringBuilder string_builder;
  string_builder.Append(NGBreakToken::ToString());
  string_builder.Append(" used:");
  string_builder.Append(used_block_size_.ToString());
  string_builder.Append("px");
  return string_builder.ToString();
}

#endif  // NDEBUG

}  // namespace blink
