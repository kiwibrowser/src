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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LINE_ABSTRACT_INLINE_TEXT_BOX_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LINE_ABSTRACT_INLINE_TEXT_BOX_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_text.h"
#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

class InlineTextBox;

// High-level abstraction of InlineTextBox to allow the accessibility module to
// get information about InlineTextBoxes without tight coupling.
class CORE_EXPORT AbstractInlineTextBox
    : public RefCounted<AbstractInlineTextBox> {
 private:
  AbstractInlineTextBox(LineLayoutText line_layout_item,
                        InlineTextBox* inline_text_box)
      : line_layout_item_(line_layout_item),
        inline_text_box_(inline_text_box) {}

  static scoped_refptr<AbstractInlineTextBox> GetOrCreate(LineLayoutText,
                                                          InlineTextBox*);
  static void WillDestroy(InlineTextBox*);

  friend class LayoutText;
  friend class InlineTextBox;

 public:
  struct WordBoundaries {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
    WordBoundaries(int start_index, int end_index)
        : start_index(start_index), end_index(end_index) {}
    int start_index;
    int end_index;
  };

  enum Direction { kLeftToRight, kRightToLeft, kTopToBottom, kBottomToTop };

  ~AbstractInlineTextBox();

  LineLayoutText GetLineLayoutItem() const { return line_layout_item_; }

  scoped_refptr<AbstractInlineTextBox> NextInlineTextBox() const;
  LayoutRect LocalBounds() const;
  unsigned Len() const;
  Direction GetDirection() const;
  Node* GetNode() const;
  void CharacterWidths(Vector<float>&) const;
  void GetWordBoundaries(Vector<WordBoundaries>&) const;
  String GetText() const;
  bool IsFirst() const;
  bool IsLast() const;
  scoped_refptr<AbstractInlineTextBox> NextOnLine() const;
  scoped_refptr<AbstractInlineTextBox> PreviousOnLine() const;

 private:
  void Detach();

  // Weak ptrs; these are nulled when InlineTextBox::destroy() calls
  // AbstractInlineTextBox::willDestroy.
  LineLayoutText line_layout_item_;
  InlineTextBox* inline_text_box_;

  typedef HashMap<InlineTextBox*, scoped_refptr<AbstractInlineTextBox>>
      InlineToAbstractInlineTextBoxHashMap;
  static InlineToAbstractInlineTextBoxHashMap* g_abstract_inline_text_box_map_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LINE_ABSTRACT_INLINE_TEXT_BOX_H_
