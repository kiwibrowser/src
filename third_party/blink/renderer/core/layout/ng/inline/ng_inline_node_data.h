// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGInlineNodeData_h
#define NGInlineNodeData_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Data which is required for inline nodes.
struct CORE_EXPORT NGInlineNodeData : NGInlineItemsData {
 private:
  const NGInlineItemsData& ItemsData(bool is_first_line) const {
    return !is_first_line || !first_line_items_
               ? (const NGInlineItemsData&)*this
               : *first_line_items_;
  }
  TextDirection BaseDirection() const {
    return static_cast<TextDirection>(base_direction_);
  }
  void SetBaseDirection(TextDirection direction) {
    base_direction_ = static_cast<unsigned>(direction);
  }

  friend class NGInlineNode;
  friend class NGInlineNodeLegacy;
  friend class NGInlineNodeForTest;
  friend class NGOffsetMappingTest;

  // Items to use for the first line, when the node has :first-line rules.
  //
  // Items have different ComputedStyle, and may also have different
  // text_content and ShapeResult if 'text-transform' is applied or fonts are
  // different.
  std::unique_ptr<NGInlineItemsData> first_line_items_;

  unsigned is_bidi_enabled_ : 1;
  unsigned base_direction_ : 1;  // TextDirection

  // We use this flag to determine if the inline node is empty, and will
  // produce a single zero block-size line box. If the node has text, atomic
  // inlines, open/close tags with margins/border/padding this will be false.
  unsigned is_empty_inline_ : 1;
};

}  // namespace blink

#endif  // NGInlineNode_h
