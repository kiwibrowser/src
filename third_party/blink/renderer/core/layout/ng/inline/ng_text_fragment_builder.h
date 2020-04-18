// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGTextFragmentBuilder_h
#define NGTextFragmentBuilder_h

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_text_end_effect.h"
#include "third_party/blink/renderer/core/layout/ng/ng_base_fragment_builder.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutObject;
class ShapeResult;
struct NGInlineItemResult;

class CORE_EXPORT NGTextFragmentBuilder final : public NGBaseFragmentBuilder {
  STACK_ALLOCATED();

 public:
  NGTextFragmentBuilder(NGInlineNode, WritingMode);

  // NOTE: Takes ownership of the shape result within the item result.
  void SetItem(NGPhysicalTextFragment::NGTextType,
               const NGInlineItemsData&,
               NGInlineItemResult*,
               LayoutUnit line_height);
  void SetText(LayoutObject*,
               const String& text,
               scoped_refptr<const ComputedStyle>,
               bool is_ellipsis_style,
               scoped_refptr<const ShapeResult>);

  // Creates the fragment. Can only be called once.
  scoped_refptr<NGPhysicalTextFragment> ToTextFragment();

 private:
  NGInlineNode inline_node_;
  String text_;
  unsigned item_index_;
  unsigned start_offset_;
  unsigned end_offset_;
  NGLogicalSize size_;
  scoped_refptr<const ShapeResult> shape_result_;

  NGPhysicalTextFragment::NGTextType text_type_ =
      NGPhysicalTextFragment::kNormalText;

  NGTextEndEffect end_effect_ = NGTextEndEffect::kNone;
  LayoutObject* layout_object_ = nullptr;
};

}  // namespace blink

#endif  // NGTextFragmentBuilder
