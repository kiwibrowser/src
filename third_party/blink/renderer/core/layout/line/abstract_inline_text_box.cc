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

#include "third_party/blink/renderer/core/layout/line/abstract_inline_text_box.h"

#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"

namespace blink {

AbstractInlineTextBox::InlineToAbstractInlineTextBoxHashMap*
    AbstractInlineTextBox::g_abstract_inline_text_box_map_ = nullptr;

scoped_refptr<AbstractInlineTextBox> AbstractInlineTextBox::GetOrCreate(
    LineLayoutText line_layout_text,
    InlineTextBox* inline_text_box) {
  if (!inline_text_box)
    return nullptr;

  if (!g_abstract_inline_text_box_map_)
    g_abstract_inline_text_box_map_ =
        new InlineToAbstractInlineTextBoxHashMap();

  InlineToAbstractInlineTextBoxHashMap::const_iterator it =
      g_abstract_inline_text_box_map_->find(inline_text_box);
  if (it != g_abstract_inline_text_box_map_->end())
    return it->value;

  scoped_refptr<AbstractInlineTextBox> obj = base::AdoptRef(
      new AbstractInlineTextBox(line_layout_text, inline_text_box));
  g_abstract_inline_text_box_map_->Set(inline_text_box, obj);
  return obj;
}

void AbstractInlineTextBox::WillDestroy(InlineTextBox* inline_text_box) {
  if (!g_abstract_inline_text_box_map_)
    return;

  InlineToAbstractInlineTextBoxHashMap::const_iterator it =
      g_abstract_inline_text_box_map_->find(inline_text_box);
  if (it != g_abstract_inline_text_box_map_->end()) {
    it->value->Detach();
    g_abstract_inline_text_box_map_->erase(inline_text_box);
  }
}

AbstractInlineTextBox::~AbstractInlineTextBox() {
  DCHECK(!line_layout_item_);
  DCHECK(!inline_text_box_);
}

void AbstractInlineTextBox::Detach() {
  if (Node* node = GetNode()) {
    if (AXObjectCache* cache = node->GetDocument().ExistingAXObjectCache())
      cache->Remove(this);
  }

  line_layout_item_ = LineLayoutText(nullptr);
  inline_text_box_ = nullptr;
}

scoped_refptr<AbstractInlineTextBox> AbstractInlineTextBox::NextInlineTextBox()
    const {
  DCHECK(!inline_text_box_ ||
         !inline_text_box_->GetLineLayoutItem().NeedsLayout());
  if (!inline_text_box_)
    return nullptr;

  return GetOrCreate(line_layout_item_,
                     inline_text_box_->NextForSameLayoutObject());
}

LayoutRect AbstractInlineTextBox::LocalBounds() const {
  if (!inline_text_box_ || !line_layout_item_)
    return LayoutRect();

  return inline_text_box_->FrameRect();
}

unsigned AbstractInlineTextBox::Len() const {
  if (!inline_text_box_)
    return 0;

  return inline_text_box_->Len();
}

AbstractInlineTextBox::Direction AbstractInlineTextBox::GetDirection() const {
  if (!inline_text_box_ || !line_layout_item_)
    return kLeftToRight;

  if (line_layout_item_.Style()->IsHorizontalWritingMode()) {
    return (inline_text_box_->Direction() == TextDirection::kRtl
                ? kRightToLeft
                : kLeftToRight);
  }
  return (inline_text_box_->Direction() == TextDirection::kRtl ? kBottomToTop
                                                               : kTopToBottom);
}

Node* AbstractInlineTextBox::GetNode() const {
  if (!line_layout_item_)
    return nullptr;
  return line_layout_item_.GetNode();
}

void AbstractInlineTextBox::CharacterWidths(Vector<float>& widths) const {
  if (!inline_text_box_)
    return;

  inline_text_box_->CharacterWidths(widths);
}

void AbstractInlineTextBox::GetWordBoundaries(
    Vector<WordBoundaries>& words) const {
  if (!inline_text_box_)
    return;

  String text = GetText();
  int len = text.length();
  TextBreakIterator* iterator = WordBreakIterator(text, 0, len);

  // FIXME: When http://crbug.com/411764 is fixed, replace this with an ASSERT.
  if (!iterator)
    return;

  int pos = iterator->first();
  while (pos >= 0 && pos < len) {
    int next = iterator->next();
    if (IsWordTextBreak(iterator))
      words.push_back(WordBoundaries(pos, next));
    pos = next;
  }
}

String AbstractInlineTextBox::GetText() const {
  if (!inline_text_box_ || !line_layout_item_)
    return String();

  unsigned start = inline_text_box_->Start();
  unsigned len = inline_text_box_->Len();
  if (Node* node = line_layout_item_.GetNode()) {
    if (node->IsTextNode()) {
      return PlainText(
          EphemeralRange(Position(node, start), Position(node, start + len)),
          TextIteratorBehavior::IgnoresStyleVisibilityBehavior());
    }
    return PlainText(
        EphemeralRange(Position(node, PositionAnchorType::kBeforeAnchor),
                       Position(node, PositionAnchorType::kAfterAnchor)),
        TextIteratorBehavior::IgnoresStyleVisibilityBehavior());
  }

  String result = line_layout_item_.GetText()
                      .Substring(start, len)
                      .SimplifyWhiteSpace(WTF::kDoNotStripWhiteSpace);
  if (inline_text_box_->NextForSameLayoutObject() &&
      inline_text_box_->NextForSameLayoutObject()->Start() >
          inline_text_box_->end() &&
      result.length() && !result.Right(1).ContainsOnlyWhitespace())
    return result + " ";
  return result;
}

bool AbstractInlineTextBox::IsFirst() const {
  DCHECK(!inline_text_box_ ||
         !inline_text_box_->GetLineLayoutItem().NeedsLayout());
  return !inline_text_box_ || !inline_text_box_->PrevForSameLayoutObject();
}

bool AbstractInlineTextBox::IsLast() const {
  DCHECK(!inline_text_box_ ||
         !inline_text_box_->GetLineLayoutItem().NeedsLayout());
  return !inline_text_box_ || !inline_text_box_->NextForSameLayoutObject();
}

scoped_refptr<AbstractInlineTextBox> AbstractInlineTextBox::NextOnLine() const {
  DCHECK(!inline_text_box_ ||
         !inline_text_box_->GetLineLayoutItem().NeedsLayout());
  if (!inline_text_box_)
    return nullptr;

  InlineBox* next = inline_text_box_->NextOnLine();
  if (next && next->IsInlineTextBox())
    return GetOrCreate(ToInlineTextBox(next)->GetLineLayoutItem(),
                       ToInlineTextBox(next));

  return nullptr;
}

scoped_refptr<AbstractInlineTextBox> AbstractInlineTextBox::PreviousOnLine()
    const {
  DCHECK(!inline_text_box_ ||
         !inline_text_box_->GetLineLayoutItem().NeedsLayout());
  if (!inline_text_box_)
    return nullptr;

  InlineBox* previous = inline_text_box_->PrevOnLine();
  if (previous && previous->IsInlineTextBox())
    return GetOrCreate(ToInlineTextBox(previous)->GetLineLayoutItem(),
                       ToInlineTextBox(previous));

  return nullptr;
}

}  // namespace blink
