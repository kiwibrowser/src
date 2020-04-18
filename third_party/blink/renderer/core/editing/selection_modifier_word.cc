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
#include "third_party/blink/renderer/core/editing/selection_modifier.h"

#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/inline_box_position.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"
#include "third_party/blink/renderer/core/layout/line/root_inline_box.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"

namespace blink {

namespace {

// This class holds a list of |InlineBox| in logical order.
// TDOO(editing-dev): We should utilize |CachedLogicallyOrderedLeafBoxes| class
// in |CompositeEditCommand::DeleteInsignificantText()|.
class CachedLogicallyOrderedLeafBoxes final {
 public:
  CachedLogicallyOrderedLeafBoxes() = default;

  const InlineTextBox* PreviousTextBox(const RootInlineBox*,
                                       const InlineTextBox*);
  const InlineTextBox* NextTextBox(const RootInlineBox*, const InlineTextBox*);

  size_t size() const { return leaf_boxes_.size(); }
  const InlineBox* FirstBox() const { return leaf_boxes_[0]; }

 private:
  const Vector<InlineBox*>& CollectBoxes(const RootInlineBox*);
  int BoxIndexInLeaves(const InlineTextBox*) const;

  const RootInlineBox* root_inline_box_ = nullptr;
  Vector<InlineBox*> leaf_boxes_;
};

const InlineTextBox* CachedLogicallyOrderedLeafBoxes::PreviousTextBox(
    const RootInlineBox* root,
    const InlineTextBox* box) {
  if (!root)
    return nullptr;

  CollectBoxes(root);

  // If box is null, root is box's previous RootInlineBox, and previousBox is
  // the last logical box in root.
  int box_index = leaf_boxes_.size() - 1;
  if (box)
    box_index = BoxIndexInLeaves(box) - 1;

  for (int i = box_index; i >= 0; --i) {
    if (leaf_boxes_[i]->IsInlineTextBox())
      return ToInlineTextBox(leaf_boxes_[i]);
  }

  return nullptr;
}

const InlineTextBox* CachedLogicallyOrderedLeafBoxes::NextTextBox(
    const RootInlineBox* root,
    const InlineTextBox* box) {
  if (!root)
    return nullptr;

  CollectBoxes(root);

  // If box is null, root is box's next RootInlineBox, and nextBox is the first
  // logical box in root. Otherwise, root is box's RootInlineBox, and nextBox is
  // the next logical box in the same line.
  size_t next_box_index = 0;
  if (box)
    next_box_index = BoxIndexInLeaves(box) + 1;

  for (size_t i = next_box_index; i < leaf_boxes_.size(); ++i) {
    if (leaf_boxes_[i]->IsInlineTextBox())
      return ToInlineTextBox(leaf_boxes_[i]);
  }

  return nullptr;
}

const Vector<InlineBox*>& CachedLogicallyOrderedLeafBoxes::CollectBoxes(
    const RootInlineBox* root) {
  if (root_inline_box_ != root) {
    root_inline_box_ = root;
    leaf_boxes_.clear();
    root->CollectLeafBoxesInLogicalOrder(leaf_boxes_);
  }
  return leaf_boxes_;
}

int CachedLogicallyOrderedLeafBoxes::BoxIndexInLeaves(
    const InlineTextBox* box) const {
  for (size_t i = 0; i < leaf_boxes_.size(); ++i) {
    if (box == leaf_boxes_[i])
      return i;
  }
  return 0;
}

const InlineTextBox* LogicallyPreviousBox(
    const VisiblePosition& visible_position,
    const InlineTextBox* text_box,
    bool& previous_box_in_different_block,
    CachedLogicallyOrderedLeafBoxes& leaf_boxes) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const InlineBox* start_box = text_box;

  const InlineTextBox* previous_box =
      leaf_boxes.PreviousTextBox(&start_box->Root(), text_box);
  if (previous_box)
    return previous_box;

  previous_box =
      leaf_boxes.PreviousTextBox(start_box->Root().PrevRootBox(), nullptr);
  if (previous_box)
    return previous_box;

  for (;;) {
    Node* start_node = start_box->GetLineLayoutItem().NonPseudoNode();
    if (!start_node)
      break;

    Position position = PreviousRootInlineBoxCandidatePosition(
        start_node, visible_position, kContentIsEditable);
    if (position.IsNull())
      break;

    const InlineBox* inline_box =
        ComputeInlineBoxPosition(
            PositionWithAffinity(position, TextAffinity::kDownstream))
            .inline_box;
    if (!inline_box)
      break;

    const RootInlineBox& previous_root = inline_box->Root();
    previous_box = leaf_boxes.PreviousTextBox(&previous_root, nullptr);
    if (previous_box) {
      previous_box_in_different_block = true;
      return previous_box;
    }

    if (!leaf_boxes.size())
      break;
    start_box = leaf_boxes.FirstBox();
  }
  return nullptr;
}

const InlineTextBox* LogicallyNextBox(
    const VisiblePosition& visible_position,
    const InlineTextBox* text_box,
    bool& next_box_in_different_block,
    CachedLogicallyOrderedLeafBoxes& leaf_boxes) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const InlineBox* start_box = text_box;

  const InlineTextBox* next_box =
      leaf_boxes.NextTextBox(&start_box->Root(), text_box);
  if (next_box)
    return next_box;

  next_box = leaf_boxes.NextTextBox(start_box->Root().NextRootBox(), nullptr);
  if (next_box)
    return next_box;

  for (;;) {
    Node* start_node = start_box->GetLineLayoutItem().NonPseudoNode();
    if (!start_node)
      break;

    Position position = NextRootInlineBoxCandidatePosition(
        start_node, visible_position, kContentIsEditable);
    if (position.IsNull())
      break;

    const InlineBox* inline_box =
        ComputeInlineBoxPosition(
            PositionWithAffinity(position, TextAffinity::kDownstream))
            .inline_box;
    if (!inline_box)
      break;

    const RootInlineBox& next_root = inline_box->Root();
    next_box = leaf_boxes.NextTextBox(&next_root, nullptr);
    if (next_box) {
      next_box_in_different_block = true;
      return next_box;
    }

    if (!leaf_boxes.size())
      break;
    start_box = leaf_boxes.FirstBox();
  }
  return nullptr;
}

TextBreakIterator* WordBreakIteratorForMinOffsetBoundary(
    const VisiblePosition& visible_position,
    const InlineTextBox* text_box,
    int& previous_box_length,
    bool& previous_box_in_different_block,
    Vector<UChar, 1024>& string,
    CachedLogicallyOrderedLeafBoxes& leaf_boxes) {
  DCHECK(visible_position.IsValid()) << visible_position;
  previous_box_in_different_block = false;

  // TODO(editing-dev) Handle the case when we don't have an inline text box.
  const InlineTextBox* previous_box = LogicallyPreviousBox(
      visible_position, text_box, previous_box_in_different_block, leaf_boxes);

  int len = 0;
  string.clear();
  if (previous_box) {
    previous_box_length = previous_box->Len();
    previous_box->GetLineLayoutItem().GetText().AppendTo(
        string, previous_box->Start(), previous_box_length);
    len += previous_box_length;
  }
  text_box->GetLineLayoutItem().GetText().AppendTo(string, text_box->Start(),
                                                   text_box->Len());
  len += text_box->Len();

  return WordBreakIterator(string.data(), len);
}

TextBreakIterator* WordBreakIteratorForMaxOffsetBoundary(
    const VisiblePosition& visible_position,
    const InlineTextBox* text_box,
    bool& next_box_in_different_block,
    Vector<UChar, 1024>& string,
    CachedLogicallyOrderedLeafBoxes& leaf_boxes) {
  DCHECK(visible_position.IsValid()) << visible_position;
  next_box_in_different_block = false;

  // TODO(editing-dev) Handle the case when we don't have an inline text box.
  const InlineTextBox* next_box = LogicallyNextBox(
      visible_position, text_box, next_box_in_different_block, leaf_boxes);

  int len = 0;
  string.clear();
  text_box->GetLineLayoutItem().GetText().AppendTo(string, text_box->Start(),
                                                   text_box->Len());
  len += text_box->Len();
  if (next_box) {
    next_box->GetLineLayoutItem().GetText().AppendTo(string, next_box->Start(),
                                                     next_box->Len());
    len += next_box->Len();
  }

  return WordBreakIterator(string.data(), len);
}

bool IsLogicalStartOfWord(TextBreakIterator* iter,
                          int position,
                          bool hard_line_break) {
  bool boundary = hard_line_break ? true : iter->isBoundary(position);
  if (!boundary)
    return false;

  // isWordTextBreak returns true after moving across a word and false after
  // moving across a punctuation/space.
  // If |iter| is already at the end before |iter->following| is called,
  // IsWordTextBreak behaves differently depending on the ICU version. We have
  // to check if |iter| is at the end, first.
  // See https://ssl.icu-project.org/trac/ticket/13447 .
  if (iter->following(position) == TextBreakIterator::DONE)
    return false;
  return IsWordTextBreak(iter);
}

bool IslogicalEndOfWord(TextBreakIterator* iter,
                        int position,
                        bool hard_line_break) {
  bool boundary = iter->isBoundary(position);
  return (hard_line_break || boundary) && IsWordTextBreak(iter);
}

enum CursorMovementDirection { kMoveLeft, kMoveRight };

VisiblePosition VisualWordPosition(const VisiblePosition& visible_position,
                                   CursorMovementDirection direction,
                                   bool skips_space_when_moving_right) {
  DCHECK(visible_position.IsValid()) << visible_position;
  if (visible_position.IsNull())
    return VisiblePosition();

  TextDirection block_direction =
      DirectionOfEnclosingBlockOf(visible_position.DeepEquivalent());
  const InlineBox* previously_visited_box = nullptr;
  VisiblePosition current = visible_position;
  TextBreakIterator* iter = nullptr;

  CachedLogicallyOrderedLeafBoxes leaf_boxes;
  Vector<UChar, 1024> string;

  for (;;) {
    VisiblePosition adjacent_character_position = direction == kMoveRight
                                                      ? RightPositionOf(current)
                                                      : LeftPositionOf(current);
    if (adjacent_character_position.DeepEquivalent() ==
            current.DeepEquivalent() ||
        adjacent_character_position.IsNull())
      return VisiblePosition();

    InlineBoxPosition box_position = ComputeInlineBoxPosition(
        PositionWithAffinity(adjacent_character_position.DeepEquivalent(),
                             TextAffinity::kUpstream));
    const InlineBox* box = box_position.inline_box;
    int offset_in_box = box_position.offset_in_box;

    if (!box)
      break;
    if (!box->IsInlineTextBox()) {
      current = adjacent_character_position;
      continue;
    }

    const InlineTextBox* text_box = ToInlineTextBox(box);
    int previous_box_length = 0;
    bool previous_box_in_different_block = false;
    bool next_box_in_different_block = false;
    bool moving_into_new_box = previously_visited_box != box;

    if (offset_in_box == box->CaretMinOffset()) {
      iter = WordBreakIteratorForMinOffsetBoundary(
          visible_position, text_box, previous_box_length,
          previous_box_in_different_block, string, leaf_boxes);
    } else if (offset_in_box == box->CaretMaxOffset()) {
      iter = WordBreakIteratorForMaxOffsetBoundary(visible_position, text_box,
                                                   next_box_in_different_block,
                                                   string, leaf_boxes);
    } else if (moving_into_new_box) {
      iter = WordBreakIterator(text_box->GetLineLayoutItem().GetText(),
                               text_box->Start(), text_box->Len());
      previously_visited_box = box;
    }

    if (!iter)
      break;

    iter->first();
    int offset_in_iterator =
        offset_in_box - text_box->Start() + previous_box_length;

    bool is_word_break;
    bool box_has_same_directionality_as_block =
        box->Direction() == block_direction;
    bool moving_backward =
        (direction == kMoveLeft && box->Direction() == TextDirection::kLtr) ||
        (direction == kMoveRight && box->Direction() == TextDirection::kRtl);
    if ((skips_space_when_moving_right &&
         box_has_same_directionality_as_block) ||
        (!skips_space_when_moving_right && moving_backward)) {
      bool logical_start_in_layout_object =
          offset_in_box == static_cast<int>(text_box->Start()) &&
          previous_box_in_different_block;
      is_word_break = IsLogicalStartOfWord(iter, offset_in_iterator,
                                           logical_start_in_layout_object);
    } else {
      bool logical_end_in_layout_object =
          offset_in_box ==
              static_cast<int>(text_box->Start() + text_box->Len()) &&
          next_box_in_different_block;
      is_word_break = IslogicalEndOfWord(iter, offset_in_iterator,
                                         logical_end_in_layout_object);
    }

    if (is_word_break) {
      return AdjustBackwardPositionToAvoidCrossingEditingBoundaries(
          adjacent_character_position, visible_position.DeepEquivalent());
    }

    current = adjacent_character_position;
  }
  return VisiblePosition();
}

}  // namespace

VisiblePosition SelectionModifier::LeftWordPosition(
    const VisiblePosition& visible_position,
    bool skips_space_when_moving_right) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const VisiblePosition& left_word_break = VisualWordPosition(
      visible_position, kMoveLeft, skips_space_when_moving_right);
  if (left_word_break.IsNotNull())
    return left_word_break;
  // TODO(editing-dev) How should we handle a non-editable position?
  if (!IsEditablePosition(visible_position.DeepEquivalent()))
    return left_word_break;
  const TextDirection block_direction =
      DirectionOfEnclosingBlockOf(visible_position.DeepEquivalent());
  return block_direction == TextDirection::kLtr
             ? StartOfEditableContent(visible_position)
             : EndOfEditableContent(visible_position);
}

VisiblePosition SelectionModifier::RightWordPosition(
    const VisiblePosition& visible_position,
    bool skips_space_when_moving_right) {
  DCHECK(visible_position.IsValid()) << visible_position;
  const VisiblePosition& right_word_break = VisualWordPosition(
      visible_position, kMoveRight, skips_space_when_moving_right);
  if (right_word_break.IsNotNull())
    return right_word_break;
  // TODO(editing-dev) How should we handle a non-editable position?
  if (!IsEditablePosition(visible_position.DeepEquivalent()))
    return right_word_break;
  const TextDirection block_direction =
      blink::DirectionOfEnclosingBlockOf(visible_position.DeepEquivalent());
  return block_direction == TextDirection::kLtr
             ? EndOfEditableContent(visible_position)
             : StartOfEditableContent(visible_position);
}

}  // namespace blink
