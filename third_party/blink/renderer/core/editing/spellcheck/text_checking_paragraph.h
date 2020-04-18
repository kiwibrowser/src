/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_TEXT_CHECKING_PARAGRAPH_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_TEXT_CHECKING_PARAGRAPH_H_

#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Range;

class TextCheckingParagraph {
  STACK_ALLOCATED();

 public:
  explicit TextCheckingParagraph(const EphemeralRange& checking_range);
  TextCheckingParagraph(const EphemeralRange& checking_range,
                        const EphemeralRange& paragraph_range);
  TextCheckingParagraph(Range* checking_range, Range* paragraph_range);
  ~TextCheckingParagraph();

  int RangeLength() const;
  EphemeralRange Subrange(int character_offset, int character_count) const;
  void ExpandRangeToNextEnd();

  const String& GetText() const;
  // Why not let clients call these functions on text() themselves?
  String TextSubstring(unsigned pos, unsigned len = INT_MAX) const {
    return GetText().Substring(pos, len);
  }
  UChar TextCharAt(int index) const {
    return GetText()[static_cast<unsigned>(index)];
  }

  bool IsEmpty() const;

  int CheckingStart() const;
  int CheckingEnd() const;
  int CheckingLength() const;

  bool CheckingRangeCovers(int location, int length) const {
    return location < CheckingEnd() && location + length > CheckingStart();
  }
  EphemeralRange ParagraphRange() const;
  void SetParagraphRange(const EphemeralRange&);

  EphemeralRange CheckingRange() const { return checking_range_; }

 private:
  void InvalidateParagraphRangeValues();
  EphemeralRange OffsetAsRange() const;

  bool IsTextEmpty() const { return GetText().IsEmpty(); }
  bool IsRangeEmpty() const { return CheckingStart() >= CheckingEnd(); }

  EphemeralRange checking_range_;
  mutable EphemeralRange paragraph_range_;
  mutable EphemeralRange offset_as_range_;
  mutable String text_;
  mutable int checking_start_;
  mutable int checking_end_;
  mutable int checking_length_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_TEXT_CHECKING_PARAGRAPH_H_
