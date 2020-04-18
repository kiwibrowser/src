/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
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

#include "third_party/blink/renderer/core/editing/selection_adjuster.h"

#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"

namespace blink {

namespace {

template <typename Strategy>
SelectionTemplate<Strategy> ComputeAdjustedSelection(
    const SelectionTemplate<Strategy> selection,
    const EphemeralRangeTemplate<Strategy>& range) {
  if (selection.ComputeRange() == range) {
    // To pass "editing/deleting/delete_after_block_image.html", we need to
    // return original selection.
    return selection;
  }
  if (range.StartPosition().CompareTo(range.EndPosition()) == 0) {
    return typename SelectionTemplate<Strategy>::Builder()
        .Collapse(selection.IsBaseFirst() ? range.StartPosition()
                                          : range.EndPosition())
        .Build();
  }
  if (selection.IsBaseFirst()) {
    return typename SelectionTemplate<Strategy>::Builder()
        .SetAsForwardSelection(range)
        .Build();
  }
  return typename SelectionTemplate<Strategy>::Builder()
      .SetAsBackwardSelection(range)
      .Build();
}

bool IsEmptyTableCell(const Node* node) {
  // Returns true IFF the passed in node is one of:
  //   .) a table cell with no children,
  //   .) a table cell with a single BR child, and which has no other child
  //      layoutObject, including :before and :after layoutObject
  //   .) the BR child of such a table cell

  // Find rendered node
  while (node && !node->GetLayoutObject())
    node = node->parentNode();
  if (!node)
    return false;

  // Make sure the rendered node is a table cell or <br>.
  // If it's a <br>, then the parent node has to be a table cell.
  const LayoutObject* layout_object = node->GetLayoutObject();
  if (layout_object->IsBR()) {
    layout_object = layout_object->Parent();
    if (!layout_object)
      return false;
  }
  if (!layout_object->IsTableCell())
    return false;

  // Check that the table cell contains no child layoutObjects except for
  // perhaps a single <br>.
  const LayoutObject* const child_layout_object =
      layout_object->SlowFirstChild();
  if (!child_layout_object)
    return true;
  if (!child_layout_object->IsBR())
    return false;
  return !child_layout_object->NextSibling();
}

}  // anonymous namespace

class GranularityAdjuster final {
  STATIC_ONLY(GranularityAdjuster);

 public:
  template <typename Strategy>
  static PositionTemplate<Strategy> ComputeStartRespectingGranularityAlgorithm(
      const PositionWithAffinityTemplate<Strategy>& passed_start,
      TextGranularity granularity) {
    DCHECK(passed_start.IsNotNull());

    switch (granularity) {
      case TextGranularity::kCharacter:
        // Don't do any expansion.
        return passed_start.GetPosition();
      case TextGranularity::kWord: {
        // General case: Select the word the caret is positioned inside of.
        // If the caret is on the word boundary, select the word according to
        // |wordSide|.
        // Edge case: If the caret is after the last word in a soft-wrapped line
        // or the last word in the document, select that last word
        // (kPreviousWordIfOnBoundary).
        // Edge case: If the caret is after the last word in a paragraph, select
        // from the the end of the last word to the line break (also
        // kNextWordIfOnBoundary);
        const VisiblePositionTemplate<Strategy> visible_start =
            CreateVisiblePosition(passed_start);
        return StartOfWord(visible_start, ChooseWordSide(visible_start))
            .DeepEquivalent();
      }
      case TextGranularity::kSentence:
        return StartOfSentence(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
      case TextGranularity::kLine:
        return StartOfLine(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
      case TextGranularity::kLineBoundary:
        return StartOfLine(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
      case TextGranularity::kParagraph: {
        const VisiblePositionTemplate<Strategy> pos =
            CreateVisiblePosition(passed_start);
        if (IsStartOfLine(pos) && IsEndOfEditableOrNonEditableContent(pos))
          return StartOfParagraph(PreviousPositionOf(pos)).DeepEquivalent();
        return StartOfParagraph(pos).DeepEquivalent();
      }
      case TextGranularity::kDocumentBoundary:
        return StartOfDocument(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
      case TextGranularity::kParagraphBoundary:
        return StartOfParagraph(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
      case TextGranularity::kSentenceBoundary:
        return StartOfSentence(CreateVisiblePosition(passed_start))
            .DeepEquivalent();
    }

    NOTREACHED();
    return passed_start.GetPosition();
  }

  template <typename Strategy>
  static PositionTemplate<Strategy> ComputeEndRespectingGranularityAlgorithm(
      const PositionTemplate<Strategy>& start,
      const PositionWithAffinityTemplate<Strategy>& passed_end,
      TextGranularity granularity) {
    DCHECK(passed_end.IsNotNull());

    switch (granularity) {
      case TextGranularity::kCharacter:
        // Don't do any expansion.
        return passed_end.GetPosition();
      case TextGranularity::kWord: {
        // General case: Select the word the caret is positioned inside of.
        // If the caret is on the word boundary, select the word according to
        // |wordSide|.
        // Edge case: If the caret is after the last word in a soft-wrapped line
        // or the last word in the document, select that last word
        // (|kPreviousWordIfOnBoundary|).
        // Edge case: If the caret is after the last word in a paragraph, select
        // from the the end of the last word to the line break (also
        // |kNextWordIfOnBoundary|);
        const VisiblePositionTemplate<Strategy> original_end =
            CreateVisiblePosition(passed_end);
        const VisiblePositionTemplate<Strategy> word_end =
            EndOfWord(original_end, ChooseWordSide(original_end));
        if (!IsEndOfParagraph(original_end))
          return word_end.DeepEquivalent();
        if (IsEmptyTableCell(start.AnchorNode()))
          return word_end.DeepEquivalent();

        // Select the paragraph break (the space from the end of a paragraph
        // to the start of the next one) to match TextEdit.
        const VisiblePositionTemplate<Strategy> end = NextPositionOf(word_end);
        Element* const table = TableElementJustBefore(end);
        if (!table) {
          if (end.IsNull())
            return word_end.DeepEquivalent();
          return end.DeepEquivalent();
        }

        if (!IsEnclosingBlock(table))
          return word_end.DeepEquivalent();

        // The paragraph break after the last paragraph in the last cell
        // of a block table ends at the start of the paragraph after the
        // table.
        const VisiblePositionTemplate<Strategy> next =
            NextPositionOf(end, kCannotCrossEditingBoundary);
        if (next.IsNull())
          return word_end.DeepEquivalent();
        return next.DeepEquivalent();
      }
      case TextGranularity::kSentence:
        return EndOfSentence(CreateVisiblePosition(passed_end))
            .DeepEquivalent();
      case TextGranularity::kLine: {
        const VisiblePositionTemplate<Strategy> end =
            EndOfLine(CreateVisiblePosition(passed_end));
        if (!IsEndOfParagraph(end))
          return end.DeepEquivalent();
        // If the end of this line is at the end of a paragraph, include the
        // space after the end of the line in the selection.
        const VisiblePositionTemplate<Strategy> next = NextPositionOf(end);
        if (next.IsNull())
          return end.DeepEquivalent();
        return next.DeepEquivalent();
      }
      case TextGranularity::kLineBoundary:
        return EndOfLine(CreateVisiblePosition(passed_end)).DeepEquivalent();
      case TextGranularity::kParagraph: {
        const VisiblePositionTemplate<Strategy> visible_paragraph_end =
            EndOfParagraph(CreateVisiblePosition(passed_end));

        // Include the "paragraph break" (the space from the end of this
        // paragraph to the start of the next one) in the selection.
        const VisiblePositionTemplate<Strategy> end =
            NextPositionOf(visible_paragraph_end);

        Element* const table = TableElementJustBefore(end);
        if (!table) {
          if (end.IsNull())
            return visible_paragraph_end.DeepEquivalent();
          return end.DeepEquivalent();
        }

        if (!IsEnclosingBlock(table)) {
          // There is no paragraph break after the last paragraph in the
          // last cell of an inline table.
          return visible_paragraph_end.DeepEquivalent();
        }

        // The paragraph break after the last paragraph in the last cell of
        // a block table ends at the start of the paragraph after the table,
        // not at the position just after the table.
        const VisiblePositionTemplate<Strategy> next =
            NextPositionOf(end, kCannotCrossEditingBoundary);
        if (next.IsNull())
          return visible_paragraph_end.DeepEquivalent();
        return next.DeepEquivalent();
      }
      case TextGranularity::kDocumentBoundary:
        return EndOfDocument(CreateVisiblePosition(passed_end))
            .DeepEquivalent();
      case TextGranularity::kParagraphBoundary:
        return EndOfParagraph(CreateVisiblePosition(passed_end))
            .DeepEquivalent();
      case TextGranularity::kSentenceBoundary:
        return EndOfSentence(CreateVisiblePosition(passed_end))
            .DeepEquivalent();
    }
    NOTREACHED();
    return passed_end.GetPosition();
  }

  template <typename Strategy>
  static SelectionTemplate<Strategy> AdjustSelection(
      const SelectionTemplate<Strategy>& canonicalized_selection,
      TextGranularity granularity) {
    const TextAffinity affinity = canonicalized_selection.Affinity();

    const PositionTemplate<Strategy> start =
        canonicalized_selection.ComputeStartPosition();
    const PositionTemplate<Strategy> new_start =
        ComputeStartRespectingGranularityAlgorithm(
            PositionWithAffinityTemplate<Strategy>(start, affinity),
            granularity);
    const PositionTemplate<Strategy> expanded_start =
        new_start.IsNotNull() ? new_start : start;

    const PositionTemplate<Strategy> end =
        canonicalized_selection.ComputeEndPosition();
    const PositionTemplate<Strategy> new_end =
        ComputeEndRespectingGranularityAlgorithm(
            expanded_start,
            PositionWithAffinityTemplate<Strategy>(end, affinity), granularity);
    const PositionTemplate<Strategy> expanded_end =
        new_end.IsNotNull() ? new_end : end;

    const EphemeralRangeTemplate<Strategy> expanded_range(expanded_start,
                                                          expanded_end);
    return ComputeAdjustedSelection(canonicalized_selection, expanded_range);
  }

 private:
  template <typename Strategy>
  static EWordSide ChooseWordSide(
      const VisiblePositionTemplate<Strategy>& position) {
    return IsEndOfEditableOrNonEditableContent(position) ||
                   (IsEndOfLine(position) && !IsStartOfLine(position) &&
                    !IsEndOfParagraph(position))
               ? kPreviousWordIfOnBoundary
               : kNextWordIfOnBoundary;
  }
};

PositionInFlatTree ComputeStartRespectingGranularity(
    const PositionInFlatTreeWithAffinity& start,
    TextGranularity granularity) {
  return GranularityAdjuster::ComputeStartRespectingGranularityAlgorithm(
      start, granularity);
}

PositionInFlatTree ComputeEndRespectingGranularity(
    const PositionInFlatTree& start,
    const PositionInFlatTreeWithAffinity& end,
    TextGranularity granularity) {
  return GranularityAdjuster::ComputeEndRespectingGranularityAlgorithm(
      start, end, granularity);
}

SelectionInDOMTree SelectionAdjuster::AdjustSelectionRespectingGranularity(
    const SelectionInDOMTree& selection,
    TextGranularity granularity) {
  return GranularityAdjuster::AdjustSelection(selection, granularity);
}

SelectionInFlatTree SelectionAdjuster::AdjustSelectionRespectingGranularity(
    const SelectionInFlatTree& selection,
    TextGranularity granularity) {
  return GranularityAdjuster::AdjustSelection(selection, granularity);
}

class ShadowBoundaryAdjuster final {
  STATIC_ONLY(ShadowBoundaryAdjuster);

 public:
  template <typename Strategy>
  static SelectionTemplate<Strategy> AdjustSelection(
      const SelectionTemplate<Strategy>& selection) {
    if (!selection.IsRange())
      return selection;

    const EphemeralRangeTemplate<Strategy> expanded_range =
        selection.ComputeRange();

    const EphemeralRangeTemplate<Strategy> shadow_adjusted_range =
        selection.IsBaseFirst()
            ? EphemeralRangeTemplate<Strategy>(
                  expanded_range.StartPosition(),
                  AdjustSelectionEndToAvoidCrossingShadowBoundaries(
                      expanded_range))
            : EphemeralRangeTemplate<Strategy>(
                  AdjustSelectionStartToAvoidCrossingShadowBoundaries(
                      expanded_range),
                  expanded_range.EndPosition());
    return ComputeAdjustedSelection(selection, shadow_adjusted_range);
  }

 private:
  static Node* EnclosingShadowHost(Node* node) {
    for (Node* runner = node; runner;
         runner = FlatTreeTraversal::Parent(*runner)) {
      if (IsShadowHost(runner))
        return runner;
    }
    return nullptr;
  }

  static bool IsEnclosedBy(const PositionInFlatTree& position,
                           const Node& node) {
    DCHECK(position.IsNotNull());
    Node* anchor_node = position.AnchorNode();
    if (anchor_node == node)
      return !position.IsAfterAnchor() && !position.IsBeforeAnchor();

    return FlatTreeTraversal::IsDescendantOf(*anchor_node, node);
  }

  static bool IsSelectionBoundary(const Node& node) {
    return IsHTMLTextAreaElement(node) || IsHTMLInputElement(node) ||
           IsHTMLSelectElement(node);
  }

  static Node* EnclosingShadowHostForStart(const PositionInFlatTree& position) {
    Node* node = position.NodeAsRangeFirstNode();
    if (!node)
      return nullptr;
    Node* shadow_host = EnclosingShadowHost(node);
    if (!shadow_host)
      return nullptr;
    if (!IsEnclosedBy(position, *shadow_host))
      return nullptr;
    return IsSelectionBoundary(*shadow_host) ? shadow_host : nullptr;
  }

  static Node* EnclosingShadowHostForEnd(const PositionInFlatTree& position) {
    Node* node = position.NodeAsRangeLastNode();
    if (!node)
      return nullptr;
    Node* shadow_host = EnclosingShadowHost(node);
    if (!shadow_host)
      return nullptr;
    if (!IsEnclosedBy(position, *shadow_host))
      return nullptr;
    return IsSelectionBoundary(*shadow_host) ? shadow_host : nullptr;
  }

  static PositionInFlatTree AdjustPositionInFlatTreeForStart(
      const PositionInFlatTree& position,
      Node* shadow_host) {
    if (IsEnclosedBy(position, *shadow_host)) {
      if (position.IsBeforeChildren())
        return PositionInFlatTree::BeforeNode(*shadow_host);
      return PositionInFlatTree::AfterNode(*shadow_host);
    }

    // We use |firstChild|'s after instead of beforeAllChildren for backward
    // compatibility. The positions are same but the anchors would be different,
    // and selection painting uses anchor nodes.
    if (Node* first_child = FlatTreeTraversal::FirstChild(*shadow_host))
      return PositionInFlatTree::BeforeNode(*first_child);
    return PositionInFlatTree();
  }

  static Position AdjustPositionForEnd(const Position& current_position,
                                       Node* start_container_node) {
    TreeScope& tree_scope = start_container_node->GetTreeScope();

    DCHECK(current_position.ComputeContainerNode()->GetTreeScope() !=
           tree_scope);

    if (Node* ancestor = tree_scope.AncestorInThisScope(
            current_position.ComputeContainerNode())) {
      if (ancestor->contains(start_container_node))
        return Position::AfterNode(*ancestor);
      return Position::BeforeNode(*ancestor);
    }

    if (Node* last_child = tree_scope.RootNode().lastChild())
      return Position::AfterNode(*last_child);

    return Position();
  }

  static PositionInFlatTree AdjustPositionInFlatTreeForEnd(
      const PositionInFlatTree& position,
      Node* shadow_host) {
    if (IsEnclosedBy(position, *shadow_host)) {
      if (position.IsAfterChildren())
        return PositionInFlatTree::AfterNode(*shadow_host);
      return PositionInFlatTree::BeforeNode(*shadow_host);
    }

    // We use |lastChild|'s after instead of afterAllChildren for backward
    // compatibility. The positions are same but the anchors would be different,
    // and selection painting uses anchor nodes.
    if (Node* last_child = FlatTreeTraversal::LastChild(*shadow_host))
      return PositionInFlatTree::AfterNode(*last_child);
    return PositionInFlatTree();
  }

  static Position AdjustPositionForStart(const Position& current_position,
                                         Node* end_container_node) {
    TreeScope& tree_scope = end_container_node->GetTreeScope();

    DCHECK(current_position.ComputeContainerNode()->GetTreeScope() !=
           tree_scope);

    if (Node* ancestor = tree_scope.AncestorInThisScope(
            current_position.ComputeContainerNode())) {
      if (ancestor->contains(end_container_node))
        return Position::BeforeNode(*ancestor);
      return Position::AfterNode(*ancestor);
    }

    if (Node* first_child = tree_scope.RootNode().firstChild())
      return Position::BeforeNode(*first_child);

    return Position();
  }

  // TODO(hajimehoshi): Checking treeScope is wrong when a node is
  // distributed, but we leave it as it is for backward compatibility.
  static bool IsCrossingShadowBoundaries(const EphemeralRange& range) {
    DCHECK(range.IsNotNull());
    return range.StartPosition().AnchorNode()->GetTreeScope() !=
           range.EndPosition().AnchorNode()->GetTreeScope();
  }

  static Position AdjustSelectionStartToAvoidCrossingShadowBoundaries(
      const EphemeralRange& range) {
    DCHECK(range.IsNotNull());
    if (!IsCrossingShadowBoundaries(range))
      return range.StartPosition();
    return AdjustPositionForStart(range.StartPosition(),
                                  range.EndPosition().ComputeContainerNode());
  }

  static Position AdjustSelectionEndToAvoidCrossingShadowBoundaries(
      const EphemeralRange& range) {
    DCHECK(range.IsNotNull());
    if (!IsCrossingShadowBoundaries(range))
      return range.EndPosition();
    return AdjustPositionForEnd(range.EndPosition(),
                                range.StartPosition().ComputeContainerNode());
  }

  static PositionInFlatTree AdjustSelectionStartToAvoidCrossingShadowBoundaries(
      const EphemeralRangeInFlatTree& range) {
    Node* const shadow_host_start =
        EnclosingShadowHostForStart(range.StartPosition());
    Node* const shadow_host_end =
        EnclosingShadowHostForEnd(range.EndPosition());
    if (shadow_host_start == shadow_host_end)
      return range.StartPosition();
    Node* const shadow_host =
        shadow_host_end ? shadow_host_end : shadow_host_start;
    return AdjustPositionInFlatTreeForStart(range.StartPosition(), shadow_host);
  }

  static PositionInFlatTree AdjustSelectionEndToAvoidCrossingShadowBoundaries(
      const EphemeralRangeInFlatTree& range) {
    Node* const shadow_host_start =
        EnclosingShadowHostForStart(range.StartPosition());
    Node* const shadow_host_end =
        EnclosingShadowHostForEnd(range.EndPosition());
    if (shadow_host_start == shadow_host_end)
      return range.EndPosition();
    Node* const shadow_host =
        shadow_host_start ? shadow_host_start : shadow_host_end;
    return AdjustPositionInFlatTreeForEnd(range.EndPosition(), shadow_host);
  }
};

SelectionInDOMTree
SelectionAdjuster::AdjustSelectionToAvoidCrossingShadowBoundaries(
    const SelectionInDOMTree& selection) {
  return ShadowBoundaryAdjuster::AdjustSelection(selection);
}
SelectionInFlatTree
SelectionAdjuster::AdjustSelectionToAvoidCrossingShadowBoundaries(
    const SelectionInFlatTree& selection) {
  return ShadowBoundaryAdjuster::AdjustSelection(selection);
}

class EditingBoundaryAdjuster final {
  STATIC_ONLY(EditingBoundaryAdjuster);

 public:
  template <typename Strategy>
  static SelectionTemplate<Strategy> AdjustSelection(
      const SelectionTemplate<Strategy>& shadow_adjusted_selection) {
    // TODO(editing-dev): Refactor w/o EphemeralRange.
    const EphemeralRangeTemplate<Strategy> shadow_adjusted_range =
        shadow_adjusted_selection.ComputeRange();
    const EphemeralRangeTemplate<Strategy> editing_adjusted_range =
        AdjustSelectionToAvoidCrossingEditingBoundaries(
            shadow_adjusted_range, shadow_adjusted_selection.Base());
    return ComputeAdjustedSelection(shadow_adjusted_selection,
                                    editing_adjusted_range);
  }

 private:
  static Element* LowestEditableAncestor(Node* node) {
    while (node) {
      if (HasEditableStyle(*node))
        return RootEditableElement(*node);
      if (IsHTMLBodyElement(*node))
        break;
      node = node->parentNode();
    }

    return nullptr;
  }

  // Returns true if |position| is editable or its lowest editable root is not
  // |base_editable_ancestor|.
  template <typename Strategy>
  static bool ShouldContinueSearchEditingBoundary(
      const PositionTemplate<Strategy>& position,
      Element* base_editable_ancestor) {
    if (position.IsNull())
      return false;
    if (IsEditablePosition(position))
      return true;
    return LowestEditableAncestor(position.ComputeContainerNode()) !=
           base_editable_ancestor;
  }

  template <typename Strategy>
  static bool ShouldAdjustPositionToAvoidCrossingEditingBoundaries(
      const PositionTemplate<Strategy>& position,
      const ContainerNode* editable_root,
      const Element* base_editable_ancestor) {
    if (editable_root)
      return true;
    Element* const editable_ancestor =
        LowestEditableAncestor(position.ComputeContainerNode());
    return editable_ancestor != base_editable_ancestor;
  }

  // The selection ends in editable content or non-editable content inside a
  // different editable ancestor, move backward until non-editable content
  // inside the same lowest editable ancestor is reached.
  template <typename Strategy>
  static PositionTemplate<Strategy>
  AdjustSelectionEndToAvoidCrossingEditingBoundaries(
      const PositionTemplate<Strategy>& end,
      ContainerNode* end_root,
      Element* base_editable_ancestor) {
    if (ShouldAdjustPositionToAvoidCrossingEditingBoundaries(
            end, end_root, base_editable_ancestor)) {
      PositionTemplate<Strategy> position =
          PreviousVisuallyDistinctCandidate(end);
      Element* shadow_ancestor =
          end_root ? end_root->OwnerShadowHost() : nullptr;
      if (position.IsNull() && shadow_ancestor)
        position = PositionTemplate<Strategy>::AfterNode(*shadow_ancestor);
      while (ShouldContinueSearchEditingBoundary(position,
                                                 base_editable_ancestor)) {
        Element* root = RootEditableElementOf(position);
        shadow_ancestor = root ? root->OwnerShadowHost() : nullptr;
        position = IsAtomicNode(position.ComputeContainerNode())
                       ? PositionTemplate<Strategy>::InParentBeforeNode(
                             *position.ComputeContainerNode())
                       : PreviousVisuallyDistinctCandidate(position);
        if (position.IsNull() && shadow_ancestor)
          position = PositionTemplate<Strategy>::AfterNode(*shadow_ancestor);
      }
      return CreateVisiblePosition(position).DeepEquivalent();
    }
    return end;
  }

  // The selection starts in editable content or non-editable content inside a
  // different editable ancestor, move forward until non-editable content inside
  // the same lowest editable ancestor is reached.
  template <typename Strategy>
  static PositionTemplate<Strategy>
  AdjustSelectionStartToAvoidCrossingEditingBoundaries(
      const PositionTemplate<Strategy>& start,
      ContainerNode* start_root,
      Element* base_editable_ancestor) {
    if (ShouldAdjustPositionToAvoidCrossingEditingBoundaries(
            start, start_root, base_editable_ancestor)) {
      PositionTemplate<Strategy> position =
          NextVisuallyDistinctCandidate(start);
      Element* shadow_ancestor =
          start_root ? start_root->OwnerShadowHost() : nullptr;
      if (position.IsNull() && shadow_ancestor)
        position = PositionTemplate<Strategy>::BeforeNode(*shadow_ancestor);
      while (ShouldContinueSearchEditingBoundary(position,
                                                 base_editable_ancestor)) {
        Element* root = RootEditableElementOf(position);
        shadow_ancestor = root ? root->OwnerShadowHost() : nullptr;
        position = IsAtomicNode(position.ComputeContainerNode())
                       ? PositionTemplate<Strategy>::InParentAfterNode(
                             *position.ComputeContainerNode())
                       : NextVisuallyDistinctCandidate(position);
        if (position.IsNull() && shadow_ancestor)
          position = PositionTemplate<Strategy>::BeforeNode(*shadow_ancestor);
      }
      return CreateVisiblePosition(position).DeepEquivalent();
    }
    return start;
  }

  template <typename Strategy>
  static EphemeralRangeTemplate<Strategy>
  AdjustSelectionToAvoidCrossingEditingBoundaries(
      const EphemeralRangeTemplate<Strategy>& range,
      const PositionTemplate<Strategy>& base) {
    DCHECK(base.IsNotNull());
    DCHECK(range.IsNotNull());

    ContainerNode* base_root = HighestEditableRoot(base);
    ContainerNode* start_root = HighestEditableRoot(range.StartPosition());
    ContainerNode* end_root = HighestEditableRoot(range.EndPosition());

    Element* base_editable_ancestor =
        LowestEditableAncestor(base.ComputeContainerNode());

    // The base, start and end are all in the same region.  No adjustment
    // necessary.
    if (base_root == start_root && base_root == end_root)
      return range;

    // The selection is based in editable content.
    if (base_root) {
      // If the start is outside the base's editable root, cap it at the start
      // of that root. If the start is in non-editable content that is inside
      // the base's editable root, put it at the first editable position after
      // start inside the base's editable root.
      PositionTemplate<Strategy> start = range.StartPosition();
      if (start_root != base_root) {
        const VisiblePositionTemplate<Strategy> first =
            FirstEditableVisiblePositionAfterPositionInRoot(start, *base_root);
        start = first.DeepEquivalent();
        if (start.IsNull()) {
          NOTREACHED();
          return {};
        }
      }
      // If the end is outside the base's editable root, cap it at the end of
      // that root. If the end is in non-editable content that is inside the
      // base's root, put it at the last editable position before the end inside
      // the base's root.
      PositionTemplate<Strategy> end = range.EndPosition();
      if (end_root != base_root) {
        const VisiblePositionTemplate<Strategy> last =
            LastEditableVisiblePositionBeforePositionInRoot(end, *base_root);
        end = last.DeepEquivalent();
        if (end.IsNull())
          end = start;
      }
      return {start, end};
    }

    // The selection is based in non-editable content.
    // FIXME: Non-editable pieces inside editable content should be atomic, in
    // the same way that editable pieces in non-editable content are atomic.
    const PositionTemplate<Strategy>& end =
        AdjustSelectionEndToAvoidCrossingEditingBoundaries(
            range.EndPosition(), end_root, base_editable_ancestor);
    if (end.IsNull()) {
      // The selection crosses an Editing boundary.  This is a
      // programmer error in the editing code.  Happy debugging!
      NOTREACHED();
      return {};
    }

    const PositionTemplate<Strategy>& start =
        AdjustSelectionStartToAvoidCrossingEditingBoundaries(
            range.StartPosition(), start_root, base_editable_ancestor);
    if (start.IsNull()) {
      // The selection crosses an Editing boundary.  This is a
      // programmer error in the editing code.  Happy debugging!
      NOTREACHED();
      return {};
    }
    return {start, end};
  }
};

SelectionInDOMTree
SelectionAdjuster::AdjustSelectionToAvoidCrossingEditingBoundaries(
    const SelectionInDOMTree& selection) {
  return EditingBoundaryAdjuster::AdjustSelection(selection);
}
SelectionInFlatTree
SelectionAdjuster::AdjustSelectionToAvoidCrossingEditingBoundaries(
    const SelectionInFlatTree& selection) {
  return EditingBoundaryAdjuster::AdjustSelection(selection);
}

class SelectionTypeAdjuster final {
  STATIC_ONLY(SelectionTypeAdjuster);

 public:
  template <typename Strategy>
  static SelectionTemplate<Strategy> AdjustSelection(
      const SelectionTemplate<Strategy>& selection) {
    if (selection.IsNone())
      return selection;
    const EphemeralRangeTemplate<Strategy>& range = selection.ComputeRange();
    DCHECK(!NeedsLayoutTreeUpdate(range.StartPosition())) << range;
    if (range.IsCollapsed() ||
        // TODO(editing-dev): Consider this canonicalization is really needed.
        MostBackwardCaretPosition(range.StartPosition()) ==
            MostBackwardCaretPosition(range.EndPosition())) {
      return typename SelectionTemplate<Strategy>::Builder()
          .Collapse(PositionWithAffinityTemplate<Strategy>(
              range.StartPosition(), selection.Affinity()))
          .Build();
    }
    // "Constrain" the selection to be the smallest equivalent range of
    // nodes. This is a somewhat arbitrary choice, but experience shows that
    // it is useful to make to make the selection "canonical" (if only for
    // purposes of comparing selections). This is an ideal point of the code
    // to do this operation, since all selection changes that result in a
    // RANGE come through here before anyone uses it.
    // TODO(editing-dev): Consider this canonicalization is really needed.
    const EphemeralRangeTemplate<Strategy> minimal_range(
        MostForwardCaretPosition(range.StartPosition()),
        MostBackwardCaretPosition(range.EndPosition()));
    if (selection.IsBaseFirst()) {
      return typename SelectionTemplate<Strategy>::Builder()
          .SetAsForwardSelection(minimal_range)
          .Build();
    }
    return typename SelectionTemplate<Strategy>::Builder()
        .SetAsBackwardSelection(minimal_range)
        .Build();
  }
};

SelectionInDOMTree SelectionAdjuster::AdjustSelectionType(
    const SelectionInDOMTree& selection) {
  return SelectionTypeAdjuster::AdjustSelection(selection);
}
SelectionInFlatTree SelectionAdjuster::AdjustSelectionType(
    const SelectionInFlatTree& selection) {
  return SelectionTypeAdjuster::AdjustSelection(selection);
}

}  // namespace blink
