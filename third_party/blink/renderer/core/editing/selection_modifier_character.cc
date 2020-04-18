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

#include "third_party/blink/renderer/core/editing/selection_modifier.h"

#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/inline_box_position.h"
#include "third_party/blink/renderer/core/editing/inline_box_traversal.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_api_shim.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_item.h"
#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"
#include "third_party/blink/renderer/core/layout/line/root_inline_box.h"

namespace blink {

namespace {

// The traversal strategy for |LeftPositionOf()|.
template <typename Strategy>
struct TraversalLeft {
  STATIC_ONLY(TraversalLeft);

  static InlineBox* BackwardLeafChildOf(const InlineBox& box) {
    return box.NextLeafChild();
  }

  static int CaretEndOffsetOf(const InlineBox& box) {
    return box.CaretRightmostOffset();
  }

  static int CaretMinOffsetOf(TextDirection direction, const InlineBox& box) {
    if (direction == TextDirection::kLtr)
      return box.CaretMinOffset();
    return box.CaretMaxOffset();
  }

  static int CaretStartOffsetOf(const InlineBox& box) {
    return box.CaretLeftmostOffset();
  }

  static const InlineBox* FindBackwardBidiRun(const InlineBox& box,
                                              unsigned bidi_level) {
    return InlineBoxTraversal::FindRightBidiRun(box, bidi_level);
  }

  static const InlineBox& FindBackwardBoundaryOfEntireBidiRun(
      const InlineBox& box,
      unsigned bidi_level) {
    return InlineBoxTraversal::FindRightBoundaryOfEntireBidiRun(box,
                                                                bidi_level);
  }

  static const InlineBox* FindForwardBidiRun(const InlineBox& box,
                                             unsigned bidi_level) {
    return InlineBoxTraversal::FindLeftBidiRun(box, bidi_level);
  }

  static const InlineBox& FindForwardBoundaryOfEntireBidiRun(
      const InlineBox& box,
      unsigned bidi_level) {
    return InlineBoxTraversal::FindLeftBoundaryOfEntireBidiRun(box, bidi_level);
  }

  static int ForwardGraphemeBoundaryOf(TextDirection direction,
                                       const Node& node,
                                       int offset) {
    if (direction == TextDirection::kLtr)
      return PreviousGraphemeBoundaryOf(node, offset);
    return NextGraphemeBoundaryOf(node, offset);
  }

  static const InlineBox* ForwardLeafChildOf(const InlineBox& box) {
    return box.PrevLeafChild();
  }

  static const InlineBox* ForwardNonPseudoLeafChildOf(const InlineBox& box) {
    for (const InlineBox* runner = ForwardLeafChildOf(box); runner;
         runner = ForwardLeafChildOf(*runner)) {
      if (runner->GetLineLayoutItem().GetNode())
        return runner;
    }
    return nullptr;
  }

  static const InlineBox* ForwardLeafChildIgnoringLineBreakOf(
      const InlineBox& box) {
    return box.PrevLeafChildIgnoringLineBreak();
  }

  static PositionTemplate<Strategy> ForwardVisuallyDistinctCandidateOf(
      TextDirection direction,
      const PositionTemplate<Strategy>& position) {
    if (direction == TextDirection::kLtr)
      return PreviousVisuallyDistinctCandidate(position);
    return NextVisuallyDistinctCandidate(position);
  }

  static VisiblePositionTemplate<Strategy> HonorEditingBoundary(
      TextDirection direction,
      const VisiblePositionTemplate<Strategy>& visible_position,
      const PositionTemplate<Strategy>& anchor) {
    if (direction == TextDirection::kLtr) {
      return AdjustBackwardPositionToAvoidCrossingEditingBoundaries(
          visible_position, anchor);
    }
    return AdjustForwardPositionToAvoidCrossingEditingBoundaries(
        visible_position, anchor);
  }

  static const InlineBox* LogicalStartBoxOf(TextDirection direction,
                                            const InlineBox& box) {
    if (direction == TextDirection::kLtr)
      return box.Root().GetLogicalStartNonPseudoBox();
    return box.Root().GetLogicalEndNonPseudoBox();
  }

  static bool IsOvershot(int offset, const InlineBox& box) {
    if (box.IsLeftToRightDirection())
      return offset < box.CaretMinOffset();
    return offset > box.CaretMaxOffset();
  }
};

// The traversal strategy for |RightPositionOf()|.
template <typename Strategy>
struct TraversalRight {
  STATIC_ONLY(TraversalRight);

  static const InlineBox* BackwardLeafChildOf(const InlineBox& box) {
    return box.PrevLeafChild();
  }

  static int CaretEndOffsetOf(const InlineBox& box) {
    return box.CaretLeftmostOffset();
  }

  static int CaretMinOffsetOf(TextDirection direction, const InlineBox& box) {
    if (direction == TextDirection::kLtr)
      return box.CaretMaxOffset();
    return box.CaretMinOffset();
  }

  static int CaretStartOffsetOf(const InlineBox& box) {
    return box.CaretRightmostOffset();
  }

  static const InlineBox* FindBackwardBidiRun(const InlineBox& box,
                                              unsigned bidi_level) {
    return InlineBoxTraversal::FindLeftBidiRun(box, bidi_level);
  }

  static const InlineBox& FindBackwardBoundaryOfEntireBidiRun(
      const InlineBox& box,
      unsigned bidi_level) {
    return InlineBoxTraversal::FindLeftBoundaryOfEntireBidiRun(box, bidi_level);
  }

  static const InlineBox* FindForwardBidiRun(const InlineBox& box,
                                             unsigned bidi_level) {
    return InlineBoxTraversal::FindRightBidiRun(box, bidi_level);
  }

  static const InlineBox& FindForwardBoundaryOfEntireBidiRun(
      const InlineBox& box,
      unsigned bidi_level) {
    return InlineBoxTraversal::FindRightBoundaryOfEntireBidiRun(box,
                                                                bidi_level);
  }

  static int ForwardGraphemeBoundaryOf(TextDirection direction,
                                       const Node& node,
                                       int offset) {
    if (direction == TextDirection::kLtr)
      return NextGraphemeBoundaryOf(node, offset);
    return PreviousGraphemeBoundaryOf(node, offset);
  }

  static const InlineBox* ForwardLeafChildOf(const InlineBox& box) {
    return box.NextLeafChild();
  }

  static const InlineBox* ForwardNonPseudoLeafChildOf(const InlineBox& box) {
    for (const InlineBox* runner = ForwardLeafChildOf(box); runner;
         runner = ForwardLeafChildOf(*runner)) {
      if (runner->GetLineLayoutItem().GetNode())
        return runner;
    }
    return nullptr;
  }

  static const InlineBox* ForwardLeafChildIgnoringLineBreakOf(
      const InlineBox& box) {
    return box.NextLeafChildIgnoringLineBreak();
  }

  static PositionTemplate<Strategy> ForwardVisuallyDistinctCandidateOf(
      TextDirection direction,
      const PositionTemplate<Strategy>& position) {
    if (direction == TextDirection::kLtr)
      return NextVisuallyDistinctCandidate(position);
    return PreviousVisuallyDistinctCandidate(position);
  }

  static VisiblePositionTemplate<Strategy> HonorEditingBoundary(
      TextDirection direction,
      const VisiblePositionTemplate<Strategy>& visible_position,
      const PositionTemplate<Strategy>& anchor) {
    if (direction == TextDirection::kLtr) {
      return AdjustForwardPositionToAvoidCrossingEditingBoundaries(
          visible_position, anchor);
    }
    return AdjustBackwardPositionToAvoidCrossingEditingBoundaries(
        visible_position, anchor);
  }

  static const InlineBox* LogicalStartBoxOf(TextDirection direction,
                                            const InlineBox& box) {
    if (direction == TextDirection::kLtr)
      return box.Root().GetLogicalEndNonPseudoBox();
    return box.Root().GetLogicalStartNonPseudoBox();
  }

  static bool IsOvershot(int offset, const InlineBox& box) {
    if (box.IsLeftToRightDirection())
      return offset > box.CaretMaxOffset();
    return offset < box.CaretMinOffset();
  }
};

template <typename Traversal>
bool IsAfterAtomicInlineOrLineBreak(const InlineBox& box, int offset) {
  if (offset != Traversal::CaretEndOffsetOf(box))
    return false;
  if (box.IsInlineTextBox() && ToInlineTextBox(box).IsLineBreak())
    return true;
  return box.GetLineLayoutItem().IsAtomicInlineLevel();
}

template <typename Traversal>
const InlineBox* LeadingBoxOfEntireSecondaryRun(const InlineBox* box) {
  const InlineBox* runner = box;
  while (true) {
    const InlineBox& backward_box =
        Traversal::FindBackwardBoundaryOfEntireBidiRun(*runner,
                                                       runner->BidiLevel());
    if (backward_box.BidiLevel() == runner->BidiLevel())
      return &backward_box;
    DCHECK_GT(backward_box.BidiLevel(), runner->BidiLevel());
    runner = &backward_box;

    const InlineBox& forward_box =
        Traversal::FindForwardBoundaryOfEntireBidiRun(*runner,
                                                      runner->BidiLevel());
    if (forward_box.BidiLevel() == runner->BidiLevel())
      return &forward_box;
    DCHECK_GT(forward_box.BidiLevel(), runner->BidiLevel());
    runner = &forward_box;
  }
}

// TODO(xiaochengh): Stop passing return value by non-const reference parameters
template <typename Traversal>
bool FindForwardBoxInPossiblyBidiContext(const InlineBox*& box,
                                         int& offset,
                                         TextDirection primary_direction) {
  const unsigned char level = box->BidiLevel();
  if (box->Direction() == primary_direction) {
    const InlineBox* const forward_box = Traversal::ForwardLeafChildOf(*box);
    if (!forward_box) {
      if (const InlineBox* logical_start =
              Traversal::LogicalStartBoxOf(primary_direction, *box)) {
        box = logical_start;
        offset = Traversal::CaretMinOffsetOf(primary_direction, *box);
      }
      return true;
    }
    if (forward_box->BidiLevel() >= level)
      return true;

    const unsigned char forward_level = forward_box->BidiLevel();
    const InlineBox* const backward_box =
        Traversal::FindBackwardBidiRun(*box, forward_level);
    if (backward_box && backward_box->BidiLevel() == forward_level)
      return true;

    box = forward_box;
    offset = Traversal::CaretEndOffsetOf(*box);
    return box->Direction() == primary_direction;
  }

  const InlineBox* const forward_non_pseudo_box =
      Traversal::ForwardNonPseudoLeafChildOf(*box);
  if (forward_non_pseudo_box) {
    box = forward_non_pseudo_box;
    offset = Traversal::CaretEndOffsetOf(*box);
    if (box->BidiLevel() > level) {
      const InlineBox* const forward_bidi_run =
          Traversal::FindForwardBidiRun(*forward_non_pseudo_box, level);
      if (!forward_bidi_run || forward_bidi_run->BidiLevel() < level)
        return false;
    }
    return true;
  }
  // Trailing edge of a secondary run. Set to the leading edge of
  // the entire run.
  box = LeadingBoxOfEntireSecondaryRun<Traversal>(box);
  offset = Traversal::CaretMinOffsetOf(primary_direction, *box);
  return true;
}

template <typename Strategy, typename Traversal>
static PositionTemplate<Strategy> TraverseInternalAlgorithm(
    const VisiblePositionTemplate<Strategy>& visible_position) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const PositionTemplate<Strategy> deep_position =
      visible_position.DeepEquivalent();
  PositionTemplate<Strategy> p = deep_position;

  if (p.IsNull())
    return PositionTemplate<Strategy>();

  const PositionTemplate<Strategy> downstream_start =
      MostForwardCaretPosition(p);
  const TextDirection primary_direction = PrimaryDirectionOf(*p.AnchorNode());
  const TextAffinity affinity = visible_position.Affinity();

  while (true) {
    InlineBoxPosition box_position = ComputeInlineBoxPosition(
        PositionWithAffinityTemplate<Strategy>(p, affinity));
    const InlineBox* box = box_position.inline_box;
    int offset = box_position.offset_in_box;
    if (!box) {
      return Traversal::ForwardVisuallyDistinctCandidateOf(primary_direction,
                                                           deep_position);
    }

    while (true) {
      if (IsAfterAtomicInlineOrLineBreak<Traversal>(*box, offset)) {
        return Traversal::ForwardVisuallyDistinctCandidateOf(box->Direction(),
                                                             deep_position);
      }

      const LineLayoutItem line_layout_item = box->GetLineLayoutItem();

      if (!line_layout_item.GetNode()) {
        box = Traversal::ForwardLeafChildOf(*box);
        if (!box) {
          return Traversal::ForwardVisuallyDistinctCandidateOf(
              primary_direction, deep_position);
        }
        offset = Traversal::CaretEndOffsetOf(*box);
        continue;
      }

      offset = Traversal::ForwardGraphemeBoundaryOf(
          box->Direction(), *line_layout_item.GetNode(), offset);

      const int caret_min_offset = box->CaretMinOffset();
      const int caret_max_offset = box->CaretMaxOffset();

      if (offset > caret_min_offset && offset < caret_max_offset)
        break;

      if (Traversal::IsOvershot(offset, *box)) {
        // Overshot forwardly.
        const InlineBox* const forward_box =
            Traversal::ForwardLeafChildIgnoringLineBreakOf(*box);
        if (!forward_box) {
          const PositionTemplate<Strategy>& forward_position =
              Traversal::ForwardVisuallyDistinctCandidateOf(
                  primary_direction, visible_position.DeepEquivalent());
          if (forward_position.IsNull())
            return PositionTemplate<Strategy>();

          const InlineBox* forward_position_box =
              ComputeInlineBoxPosition(PositionWithAffinityTemplate<Strategy>(
                                           forward_position, affinity))
                  .inline_box;
          if (forward_position_box &&
              forward_position_box->Root() == box->Root())
            return PositionTemplate<Strategy>();
          return forward_position;
        }

        // Reposition at the other logical position corresponding to our
        // edge's visual position and go for another round.
        box = forward_box;
        offset = Traversal::CaretEndOffsetOf(*forward_box);
        continue;
      }

      DCHECK_EQ(offset, Traversal::CaretStartOffsetOf(*box));
      const bool should_break = FindForwardBoxInPossiblyBidiContext<Traversal>(
          box, offset, primary_direction);
      if (should_break)
        break;
    }

    p = PositionTemplate<Strategy>::EditingPositionOf(
        box->GetLineLayoutItem().GetNode(), offset);

    if ((IsVisuallyEquivalentCandidate(p) &&
         MostForwardCaretPosition(p) != downstream_start) ||
        p.AtStartOfTree() || p.AtEndOfTree())
      return p;

    DCHECK_NE(p, deep_position);
  }
}

template <typename Strategy, typename Traversal>
VisiblePositionTemplate<Strategy> TraverseAlgorithm(
    const VisiblePositionTemplate<Strategy>& visible_position) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const PositionTemplate<Strategy> pos =
      TraverseInternalAlgorithm<Strategy, Traversal>(visible_position);
  // TODO(yosin) Why can't we move left from the last position in a tree?
  if (pos.AtStartOfTree() || pos.AtEndOfTree())
    return VisiblePositionTemplate<Strategy>();

  const VisiblePositionTemplate<Strategy> result = CreateVisiblePosition(pos);
  DCHECK_NE(result.DeepEquivalent(), visible_position.DeepEquivalent());

  return Traversal::HonorEditingBoundary(
      DirectionOfEnclosingBlockOf(result.DeepEquivalent()), result,
      visible_position.DeepEquivalent());
}

}  // namespace

VisiblePosition LeftPositionOf(const VisiblePosition& visible_position) {
  return TraverseAlgorithm<EditingStrategy, TraversalLeft<EditingStrategy>>(
      visible_position);
}

VisiblePositionInFlatTree LeftPositionOf(
    const VisiblePositionInFlatTree& visible_position) {
  return TraverseAlgorithm<EditingInFlatTreeStrategy,
                           TraversalLeft<EditingInFlatTreeStrategy>>(
      visible_position);
}

VisiblePosition RightPositionOf(const VisiblePosition& visible_position) {
  return TraverseAlgorithm<EditingStrategy, TraversalRight<EditingStrategy>>(
      visible_position);
}

VisiblePositionInFlatTree RightPositionOf(
    const VisiblePositionInFlatTree& visible_position) {
  return TraverseAlgorithm<EditingInFlatTreeStrategy,
                           TraversalRight<EditingInFlatTreeStrategy>>(
      visible_position);
}

}  // namespace blink
