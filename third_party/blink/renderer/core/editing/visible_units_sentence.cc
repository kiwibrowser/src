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

#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"

namespace blink {

namespace {

unsigned EndSentenceBoundary(const UChar* characters,
                             unsigned length,
                             unsigned,
                             BoundarySearchContextAvailability,
                             bool&) {
  TextBreakIterator* iterator = SentenceBreakIterator(characters, length);
  return iterator->next();
}

unsigned NextSentencePositionBoundary(const UChar* characters,
                                      unsigned length,
                                      unsigned,
                                      BoundarySearchContextAvailability,
                                      bool&) {
  // FIXME: This is identical to endSentenceBoundary. This isn't right, it needs
  // to move to the equivlant position in the following sentence.
  TextBreakIterator* iterator = SentenceBreakIterator(characters, length);
  return iterator->following(0);
}

unsigned PreviousSentencePositionBoundary(const UChar* characters,
                                          unsigned length,
                                          unsigned,
                                          BoundarySearchContextAvailability,
                                          bool&) {
  // FIXME: This is identical to startSentenceBoundary. I'm pretty sure that's
  // not right.
  TextBreakIterator* iterator = SentenceBreakIterator(characters, length);
  // FIXME: The following function can return -1; we don't handle that.
  return iterator->preceding(length);
}

unsigned StartSentenceBoundary(const UChar* characters,
                               unsigned length,
                               unsigned,
                               BoundarySearchContextAvailability,
                               bool&) {
  TextBreakIterator* iterator = SentenceBreakIterator(characters, length);
  // FIXME: The following function can return -1; we don't handle that.
  return iterator->preceding(length);
}

// TODO(yosin) This includes the space after the punctuation that marks the end
// of the sentence.
template <typename Strategy>
static VisiblePositionTemplate<Strategy> EndOfSentenceAlgorithm(
    const VisiblePositionTemplate<Strategy>& c) {
  DCHECK(c.IsValid()) << c;
  return CreateVisiblePosition(NextBoundary(c, EndSentenceBoundary),
                               TextAffinity::kUpstreamIfPossible);
}

template <typename Strategy>
VisiblePositionTemplate<Strategy> StartOfSentenceAlgorithm(
    const VisiblePositionTemplate<Strategy>& c) {
  DCHECK(c.IsValid()) << c;
  return CreateVisiblePosition(PreviousBoundary(c, StartSentenceBoundary));
}

}  // namespace

VisiblePosition EndOfSentence(const VisiblePosition& c) {
  return EndOfSentenceAlgorithm<EditingStrategy>(c);
}

VisiblePositionInFlatTree EndOfSentence(const VisiblePositionInFlatTree& c) {
  return EndOfSentenceAlgorithm<EditingInFlatTreeStrategy>(c);
}

EphemeralRange ExpandEndToSentenceBoundary(const EphemeralRange& range) {
  DCHECK(range.IsNotNull());
  const VisiblePosition& visible_end =
      CreateVisiblePosition(range.EndPosition());
  DCHECK(visible_end.IsNotNull());
  const Position& sentence_end = EndOfSentence(visible_end).DeepEquivalent();
  // TODO(editing-dev): |sentenceEnd < range.endPosition()| is possible,
  // which would trigger a DCHECK in EphemeralRange's constructor if we return
  // it directly. However, this shouldn't happen and needs to be fixed.
  return EphemeralRange(
      range.StartPosition(),
      sentence_end.IsNotNull() && sentence_end > range.EndPosition()
          ? sentence_end
          : range.EndPosition());
}

EphemeralRange ExpandRangeToSentenceBoundary(const EphemeralRange& range) {
  DCHECK(range.IsNotNull());
  const VisiblePosition& visible_start =
      CreateVisiblePosition(range.StartPosition());
  DCHECK(visible_start.IsNotNull());
  const Position& sentence_start =
      StartOfSentence(visible_start).DeepEquivalent();
  // TODO(editing-dev): |sentenceStart > range.startPosition()| is possible,
  // which would trigger a DCHECK in EphemeralRange's constructor if we return
  // it directly. However, this shouldn't happen and needs to be fixed.
  return ExpandEndToSentenceBoundary(EphemeralRange(
      sentence_start.IsNotNull() && sentence_start < range.StartPosition()
          ? sentence_start
          : range.StartPosition(),
      range.EndPosition()));
}

VisiblePosition NextSentencePosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  VisiblePosition next =
      CreateVisiblePosition(NextBoundary(c, NextSentencePositionBoundary),
                            TextAffinity::kUpstreamIfPossible);
  return AdjustForwardPositionToAvoidCrossingEditingBoundaries(
      next, c.DeepEquivalent());
}

VisiblePosition PreviousSentencePosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  VisiblePosition prev = CreateVisiblePosition(
      PreviousBoundary(c, PreviousSentencePositionBoundary));
  return AdjustBackwardPositionToAvoidCrossingEditingBoundaries(
      prev, c.DeepEquivalent());
}

VisiblePosition StartOfSentence(const VisiblePosition& c) {
  return StartOfSentenceAlgorithm<EditingStrategy>(c);
}

VisiblePositionInFlatTree StartOfSentence(const VisiblePositionInFlatTree& c) {
  return StartOfSentenceAlgorithm<EditingInFlatTreeStrategy>(c);
}

}  // namespace blink
