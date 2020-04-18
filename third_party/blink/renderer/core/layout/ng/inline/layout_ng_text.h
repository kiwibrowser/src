// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_LAYOUT_NG_TEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_LAYOUT_NG_TEXT_H_

#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item.h"

namespace blink {

class NGInlineItem;

// This overrides the default LayoutText to reference LayoutNGInlineItems
// instead of InlineTextBoxes.
//
// ***** INLINE ITEMS OWNERSHIP *****
// NGInlineItems in items_ are not owned by LayoutText but are pointers into the
// LayoutNGBlockFlow's items_. Should not be accessed outside of layout.
class CORE_EXPORT LayoutNGText : public LayoutText {
 public:
  LayoutNGText(Node* node, scoped_refptr<StringImpl> text)
      : LayoutText(node, text) {}

  bool IsOfType(LayoutObjectType type) const override {
    return type == kLayoutObjectNGText || LayoutText::IsOfType(type);
  }

  bool HasValidLayout() const { return valid_ng_items_; }
  const Vector<NGInlineItem*>& InlineItems() const {
    DCHECK(valid_ng_items_);
    return inline_items_;
  }

  // Inline items depends on context. It needs to be invalidated not only when
  // it was inserted/changed but also it was moved.
  void InvalidateInlineItems() { valid_ng_items_ = false; }

  void ClearInlineItems() {
    inline_items_.clear();
    valid_ng_items_ = false;
  }

  void AddInlineItem(NGInlineItem* item) {
    inline_items_.push_back(item);
    valid_ng_items_ = true;
  }

 protected:
  void InsertedIntoTree() override {
    valid_ng_items_ = false;
    LayoutText::InsertedIntoTree();
  }

 private:
  Vector<NGInlineItem*> inline_items_;
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutNGText, IsLayoutNGText());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_LAYOUT_NG_TEXT_H_
