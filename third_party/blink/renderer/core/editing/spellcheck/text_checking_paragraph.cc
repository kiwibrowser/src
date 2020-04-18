/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/editing/spellcheck/text_checking_paragraph.h"

#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/editing/iterators/character_iterator.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"

namespace blink {

TextCheckingParagraph::TextCheckingParagraph(
    const EphemeralRange& checking_range)
    : checking_range_(checking_range),
      checking_start_(-1),
      checking_end_(-1),
      checking_length_(-1) {}

TextCheckingParagraph::TextCheckingParagraph(
    const EphemeralRange& checking_range,
    const EphemeralRange& paragraph_range)
    : checking_range_(checking_range),
      paragraph_range_(paragraph_range),
      checking_start_(-1),
      checking_end_(-1),
      checking_length_(-1) {}

TextCheckingParagraph::TextCheckingParagraph(Range* checking_range,
                                             Range* paragraph_range)
    : checking_range_(checking_range),
      paragraph_range_(paragraph_range),
      checking_start_(-1),
      checking_end_(-1),
      checking_length_(-1) {}

TextCheckingParagraph::~TextCheckingParagraph() = default;

void TextCheckingParagraph::ExpandRangeToNextEnd() {
  DCHECK(checking_range_.IsNotNull());
  SetParagraphRange(
      EphemeralRange(ParagraphRange().StartPosition(),
                     EndOfParagraph(StartOfNextParagraph(CreateVisiblePosition(
                                        ParagraphRange().StartPosition())))
                         .DeepEquivalent()));
  InvalidateParagraphRangeValues();
}

void TextCheckingParagraph::InvalidateParagraphRangeValues() {
  checking_start_ = checking_end_ = -1;
  offset_as_range_ = EphemeralRange();
  text_ = String();
}

int TextCheckingParagraph::RangeLength() const {
  DCHECK(checking_range_.IsNotNull());
  return TextIterator::RangeLength(ParagraphRange());
}

EphemeralRange TextCheckingParagraph::ParagraphRange() const {
  DCHECK(checking_range_.IsNotNull());
  if (paragraph_range_.IsNull())
    paragraph_range_ = ExpandToParagraphBoundary(CheckingRange());
  return paragraph_range_;
}

void TextCheckingParagraph::SetParagraphRange(const EphemeralRange& range) {
  paragraph_range_ = range;
}

EphemeralRange TextCheckingParagraph::Subrange(int character_offset,
                                               int character_count) const {
  DCHECK(checking_range_.IsNotNull());
  return CalculateCharacterSubrange(ParagraphRange(), character_offset,
                                    character_count);
}

bool TextCheckingParagraph::IsEmpty() const {
  // Both predicates should have same result, but we check both just to be sure.
  // We need to investigate to remove this redundancy.
  return IsRangeEmpty() || IsTextEmpty();
}

EphemeralRange TextCheckingParagraph::OffsetAsRange() const {
  DCHECK(checking_range_.IsNotNull());
  if (offset_as_range_.IsNotNull())
    return offset_as_range_;
  const Position& paragraph_start = ParagraphRange().StartPosition();
  const Position& checking_start = CheckingRange().StartPosition();
  if (paragraph_start <= checking_start) {
    offset_as_range_ = EphemeralRange(paragraph_start, checking_start);
    return offset_as_range_;
  }
  // editing/pasteboard/paste-table-001.html and more reach here.
  offset_as_range_ = EphemeralRange(checking_start, paragraph_start);
  return offset_as_range_;
}

const String& TextCheckingParagraph::GetText() const {
  DCHECK(checking_range_.IsNotNull());
  if (text_.IsEmpty())
    text_ = PlainText(ParagraphRange());
  return text_;
}

int TextCheckingParagraph::CheckingStart() const {
  DCHECK(checking_range_.IsNotNull());
  if (checking_start_ == -1)
    checking_start_ = TextIterator::RangeLength(OffsetAsRange());
  return checking_start_;
}

int TextCheckingParagraph::CheckingEnd() const {
  DCHECK(checking_range_.IsNotNull());
  if (checking_end_ == -1) {
    checking_end_ =
        CheckingStart() + TextIterator::RangeLength(CheckingRange());
  }
  return checking_end_;
}

int TextCheckingParagraph::CheckingLength() const {
  DCHECK(checking_range_.IsNotNull());
  if (-1 == checking_length_)
    checking_length_ = TextIterator::RangeLength(
        CheckingRange().StartPosition(), CheckingRange().EndPosition());
  return checking_length_;
}

}  // namespace blink
