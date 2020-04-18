/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
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

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/visible_units.h"

#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/text_offset_mapping.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/text/text_boundaries.h"

namespace blink {

namespace {

unsigned EndWordBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  DCHECK_LE(offset, length);
  if (may_have_more_context &&
      EndOfFirstWordBoundaryContext(characters + offset, length - offset) ==
          static_cast<int>(length - offset)) {
    need_more_context = true;
    return length;
  }
  need_more_context = false;
  return FindWordEndBoundary(characters, length, offset);
}

template <typename Strategy>
PositionTemplate<Strategy> EndOfWordAlgorithm(
    const VisiblePositionTemplate<Strategy>& c,
    EWordSide side) {
  DCHECK(c.IsValid()) << c;
  VisiblePositionTemplate<Strategy> p = c;
  if (side == kPreviousWordIfOnBoundary) {
    if (IsStartOfParagraph(c))
      return c.DeepEquivalent();

    p = PreviousPositionOf(c);
    if (p.IsNull())
      return c.DeepEquivalent();
  } else if (IsEndOfParagraph(c)) {
    return c.DeepEquivalent();
  }

  return NextBoundary(p, EndWordBoundary);
}

PositionInFlatTree NextWordPositionInternal(
    const PositionInFlatTree& position) {
  DCHECK(position.IsNotNull());
  PositionInFlatTree last_position = position;
  for (const auto& inline_contents :
       TextOffsetMapping::ForwardRangeOf(position)) {
    const TextOffsetMapping mapping(inline_contents);
    const String text = mapping.GetText();
    const int offset =
        last_position == position ? mapping.ComputeTextOffset(position) : 0;
    const int word_end =
        FindNextWordForward(text.Characters16(), text.length(), offset);
    if (offset < word_end)
      return mapping.GetPositionAfter(word_end);
    last_position = mapping.GetRange().EndPosition();
  }
  return last_position;
}

unsigned PreviousWordPositionBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  if (may_have_more_context &&
      !StartOfLastWordBoundaryContext(characters, offset)) {
    need_more_context = true;
    return 0;
  }
  need_more_context = false;
  return FindNextWordBackward(characters, length, offset);
}

unsigned StartWordBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  TRACE_EVENT0("blink", "startWordBoundary");
  DCHECK(offset);
  if (may_have_more_context &&
      !StartOfLastWordBoundaryContext(characters, offset)) {
    need_more_context = true;
    return 0;
  }
  need_more_context = false;
  U16_BACK_1(characters, 0, offset);
  return FindWordStartBoundary(characters, length, offset);
}

template <typename Strategy>
PositionTemplate<Strategy> StartOfWordAlgorithm(
    const VisiblePositionTemplate<Strategy>& c,
    EWordSide side) {
  DCHECK(c.IsValid()) << c;
  // TODO(yosin) This returns a null VP for c at the start of the document
  // and |side| == |kPreviousWordIfOnBoundary|
  VisiblePositionTemplate<Strategy> p = c;
  if (side == kNextWordIfOnBoundary) {
    // at paragraph end, the startofWord is the current position
    if (IsEndOfParagraph(c))
      return c.DeepEquivalent();

    p = NextPositionOf(c);
    if (p.IsNull())
      return c.DeepEquivalent();
  }
  return PreviousBoundary(p, StartWordBoundary);
}

}  // namespace

Position EndOfWordPosition(const VisiblePosition& position, EWordSide side) {
  return EndOfWordAlgorithm<EditingStrategy>(position, side);
}

VisiblePosition EndOfWord(const VisiblePosition& position, EWordSide side) {
  return CreateVisiblePosition(EndOfWordPosition(position, side),
                               TextAffinity::kUpstreamIfPossible);
}

PositionInFlatTree EndOfWordPosition(const VisiblePositionInFlatTree& position,
                                     EWordSide side) {
  return EndOfWordAlgorithm<EditingInFlatTreeStrategy>(position, side);
}

VisiblePositionInFlatTree EndOfWord(const VisiblePositionInFlatTree& position,
                                    EWordSide side) {
  return CreateVisiblePosition(EndOfWordPosition(position, side),
                               TextAffinity::kUpstreamIfPossible);
}

// ----
// TODO(editing-dev): Because of word boundary can not be an upstream position,
// we should make this function to return |PositionInFlatTree|.
PositionInFlatTreeWithAffinity NextWordPosition(
    const PositionInFlatTree& start) {
  const PositionInFlatTree next = NextWordPositionInternal(start);
  // Note: The word boundary can not be upstream position.
  const PositionInFlatTreeWithAffinity adjusted =
      AdjustForwardPositionToAvoidCrossingEditingBoundaries(
          PositionInFlatTreeWithAffinity(next), start);
  DCHECK_EQ(adjusted.Affinity(), TextAffinity::kDownstream);
  return adjusted;
}

PositionWithAffinity NextWordPosition(const Position& start) {
  const PositionInFlatTreeWithAffinity& next =
      NextWordPosition(ToPositionInFlatTree(start));
  return ToPositionInDOMTreeWithAffinity(next);
}

// TODO(yosin): This function will be removed by replacing call sites to use
// |Position| version. since there are only two call sites, one is in test.
VisiblePosition NextWordPosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  return CreateVisiblePosition(NextWordPosition(c.DeepEquivalent()));
}

VisiblePosition PreviousWordPosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  VisiblePosition prev =
      CreateVisiblePosition(PreviousBoundary(c, PreviousWordPositionBoundary));
  return AdjustBackwardPositionToAvoidCrossingEditingBoundaries(
      prev, c.DeepEquivalent());
}

Position StartOfWordPosition(const VisiblePosition& position, EWordSide side) {
  return StartOfWordAlgorithm<EditingStrategy>(position, side);
}

VisiblePosition StartOfWord(const VisiblePosition& position, EWordSide side) {
  return CreateVisiblePosition(StartOfWordPosition(position, side));
}

PositionInFlatTree StartOfWordPosition(
    const VisiblePositionInFlatTree& position,
    EWordSide side) {
  return StartOfWordAlgorithm<EditingInFlatTreeStrategy>(position, side);
}

VisiblePositionInFlatTree StartOfWord(const VisiblePositionInFlatTree& position,
                                      EWordSide side) {
  return CreateVisiblePosition(StartOfWordPosition(position, side));
}

}  // namespace blink
