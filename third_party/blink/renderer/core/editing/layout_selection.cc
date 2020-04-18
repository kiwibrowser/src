/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
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

#include "third_party/blink/renderer/core/editing/layout_selection.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/html/forms/text_control_element.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

SelectionPaintRange::SelectionPaintRange(LayoutObject* start_layout_object,
                                         base::Optional<unsigned> start_offset,
                                         LayoutObject* end_layout_object,
                                         base::Optional<unsigned> end_offset)
    : start_layout_object_(start_layout_object),
      start_offset_(start_offset),
      end_layout_object_(end_layout_object),
      end_offset_(end_offset) {}

bool SelectionPaintRange::operator==(const SelectionPaintRange& other) const {
  return start_layout_object_ == other.start_layout_object_ &&
         start_offset_ == other.start_offset_ &&
         end_layout_object_ == other.end_layout_object_ &&
         end_offset_ == other.end_offset_;
}

LayoutObject* SelectionPaintRange::StartLayoutObject() const {
  DCHECK(!IsNull());
  return start_layout_object_;
}

base::Optional<unsigned> SelectionPaintRange::StartOffset() const {
  DCHECK(!IsNull());
  return start_offset_;
}

LayoutObject* SelectionPaintRange::EndLayoutObject() const {
  DCHECK(!IsNull());
  return end_layout_object_;
}

base::Optional<unsigned> SelectionPaintRange::EndOffset() const {
  DCHECK(!IsNull());
  return end_offset_;
}

SelectionPaintRange::Iterator::Iterator(const SelectionPaintRange* range) {
  if (!range || range->IsNull()) {
    current_ = nullptr;
    return;
  }
  current_ = range->StartLayoutObject();
  stop_ = range->EndLayoutObject()->NextInPreOrder();
}

LayoutObject* SelectionPaintRange::Iterator::operator*() const {
  DCHECK(current_);
  return current_;
}

SelectionPaintRange::Iterator& SelectionPaintRange::Iterator::operator++() {
  DCHECK(current_);
  current_ = current_->NextInPreOrder();
  if (current_ && current_ != stop_)
    return *this;

  current_ = nullptr;
  return *this;
}

LayoutSelection::LayoutSelection(FrameSelection& frame_selection)
    : frame_selection_(&frame_selection),
      has_pending_selection_(false),
      paint_range_(SelectionPaintRange()) {}

enum class SelectionMode {
  kNone,
  kRange,
  kBlockCursor,
};

static SelectionMode ComputeSelectionMode(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsRange())
    return SelectionMode::kRange;
  DCHECK(selection_in_dom.IsCaret());
  if (!frame_selection.ShouldShowBlockCursor())
    return SelectionMode::kNone;
  if (IsLogicalEndOfLine(CreateVisiblePosition(selection_in_dom.Base())))
    return SelectionMode::kNone;
  return SelectionMode::kBlockCursor;
}

static EphemeralRangeInFlatTree CalcSelectionInFlatTree(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  switch (ComputeSelectionMode(frame_selection)) {
    case SelectionMode::kNone:
      return {};
    case SelectionMode::kRange: {
      const PositionInFlatTree& base =
          ToPositionInFlatTree(selection_in_dom.Base());
      const PositionInFlatTree& extent =
          ToPositionInFlatTree(selection_in_dom.Extent());
      if (base.IsNull() || extent.IsNull() || base == extent ||
          !base.IsValidFor(frame_selection.GetDocument()) ||
          !extent.IsValidFor(frame_selection.GetDocument()))
        return {};
      return base <= extent ? EphemeralRangeInFlatTree(base, extent)
                            : EphemeralRangeInFlatTree(extent, base);
    }
    case SelectionMode::kBlockCursor: {
      const PositionInFlatTree& base =
          CreateVisiblePosition(ToPositionInFlatTree(selection_in_dom.Base()))
              .DeepEquivalent();
      if (base.IsNull())
        return {};
      const PositionInFlatTree end_position =
          NextPositionOf(base, PositionMoveType::kGraphemeCluster);
      if (end_position.IsNull())
        return {};
      return base <= end_position
                 ? EphemeralRangeInFlatTree(base, end_position)
                 : EphemeralRangeInFlatTree(end_position, base);
    }
  }
  NOTREACHED();
  return {};
}

// LayoutObjects each has SelectionState of kStart, kEnd, kStartAndEnd, or
// kInside.
using SelectedLayoutObjects = HashSet<LayoutObject*>;
// OldSelectedLayoutObjects is current selected LayoutObjects with
// current SelectionState which is kStart, kEnd, kStartAndEnd or kInside.
using OldSelectedLayoutObjects = HashMap<LayoutObject*, SelectionState>;

#ifndef NDEBUG
void PrintSelectedLayoutObjects(
    const SelectedLayoutObjects& new_selected_objects) {
  std::stringstream stream;
  stream << std::endl;
  for (LayoutObject* layout_object : new_selected_objects) {
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << std::endl;
  }
  LOG(INFO) << stream.str();
}

void PrintOldSelectedLayoutObjects(
    const OldSelectedLayoutObjects& old_selected_objects) {
  std::stringstream stream;
  stream << std::endl;
  for (const auto& key_pair : old_selected_objects) {
    LayoutObject* layout_object = key_pair.key;
    SelectionState old_state = key_pair.value;
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << " old: " << old_state << std::endl;
  }
  LOG(INFO) << stream.str();
}

void PrintSelectionPaintRange(const SelectionPaintRange& paint_range) {
  std::stringstream stream;
  stream << std::endl << "layout_objects:" << std::endl;
  for (LayoutObject* layout_object : paint_range) {
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << std::endl;
  }
  LOG(INFO) << stream.str();
}

void PrintSelectionStateInLayoutView(const FrameSelection& selection) {
  std::stringstream stream;
  stream << std::endl << "layout_objects:" << std::endl;
  LayoutView* layout_view = selection.GetDocument().GetLayoutView();
  for (LayoutObject* layout_object = layout_view; layout_object;
       layout_object = layout_object->NextInPreOrder()) {
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << std::endl;
  }
  LOG(INFO) << stream.str();
}
#endif

// This class represents a selection range in layout tree and each LayoutObject
// is SelectionState-marked.
class NewPaintRangeAndSelectedLayoutObjects {
  STACK_ALLOCATED();

 public:
  NewPaintRangeAndSelectedLayoutObjects() = default;
  NewPaintRangeAndSelectedLayoutObjects(SelectionPaintRange paint_range,
                                        SelectedLayoutObjects selected_objects)
      : paint_range_(paint_range),
        selected_objects_(std::move(selected_objects)) {}
  NewPaintRangeAndSelectedLayoutObjects(
      NewPaintRangeAndSelectedLayoutObjects&& other) {
    paint_range_ = other.paint_range_;
    selected_objects_ = std::move(other.selected_objects_);
  }

  SelectionPaintRange PaintRange() const { return paint_range_; }

  const SelectedLayoutObjects& LayoutObjects() const {
    return selected_objects_;
  }

 private:
  SelectionPaintRange paint_range_;
  SelectedLayoutObjects selected_objects_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NewPaintRangeAndSelectedLayoutObjects);
};

static void SetShouldInvalidateIfNeeded(LayoutObject* layout_object) {
  if (layout_object->ShouldInvalidateSelection())
    return;
  layout_object->SetShouldInvalidateSelection();

  // We should invalidate if ancestor of |layout_object| is LayoutSVGText
  // because SVGRootInlineBoxPainter::Paint() paints selection for
  // |layout_object| in/ LayoutSVGText and it is invoked when parent
  // LayoutSVGText is invalidated.
  // That is different from InlineTextBoxPainter::Paint() which paints
  // LayoutText selection when LayoutText is invalidated.
  if (!layout_object->IsSVG())
    return;
  for (LayoutObject* parent = layout_object->Parent(); parent;
       parent = parent->Parent()) {
    if (parent->IsSVGRoot())
      return;
    if (parent->IsSVGText()) {
      if (!parent->ShouldInvalidateSelection())
        parent->SetShouldInvalidateSelection();
      return;
    }
  }
}

static void SetSelectionStateIfNeeded(LayoutObject* layout_object,
                                      SelectionState state) {
  DCHECK_NE(state, SelectionState::kContain) << layout_object;
  DCHECK_NE(state, SelectionState::kNone) << layout_object;
  if (layout_object->GetSelectionState() == state)
    return;
  layout_object->SetSelectionState(state);

  // Set containing block SelectionState kContain for CSS ::selection style.
  // See LayoutObject::InvalidatePaintForSelection().
  for (LayoutObject* containing_block = layout_object->ContainingBlock();
       containing_block;
       containing_block = containing_block->ContainingBlock()) {
    if (containing_block->GetSelectionState() == SelectionState::kContain)
      return;
    containing_block->LayoutObject::SetSelectionState(SelectionState::kContain);
  }
}

// Set ShouldInvalidateSelection flag of LayoutObjects
// comparing them in |new_range| and |old_range|.
static void SetShouldInvalidateSelection(
    const NewPaintRangeAndSelectedLayoutObjects& new_range,
    const SelectionPaintRange& old_range,
    const OldSelectedLayoutObjects& old_selected_objects) {
  // We invalidate each LayoutObject in new SelectionPaintRange which
  // has SelectionState of kStart, kEnd, kStartAndEnd, or kInside
  // and is not in old SelectionPaintRange.
  for (LayoutObject* layout_object : new_range.LayoutObjects()) {
    if (old_selected_objects.Contains(layout_object))
      continue;
    const SelectionState new_state = layout_object->GetSelectionState();
    DCHECK_NE(new_state, SelectionState::kContain) << layout_object;
    DCHECK_NE(new_state, SelectionState::kNone) << layout_object;
    SetShouldInvalidateIfNeeded(layout_object);
  }
  // For LayoutObject in old SelectionPaintRange, we invalidate LayoutObjects
  // each of:
  // 1. LayoutObject was painted and would not be painted.
  // 2. LayoutObject was not painted and would be painted.
  for (const auto& key_value : old_selected_objects) {
    LayoutObject* const layout_object = key_value.key;
    const SelectionState old_state = key_value.value;
    const SelectionState new_state = layout_object->GetSelectionState();
    if (new_state == old_state)
      continue;
    DCHECK(new_state != SelectionState::kNone ||
           old_state != SelectionState::kNone)
        << layout_object;
    DCHECK_NE(new_state, SelectionState::kContain) << layout_object;
    DCHECK_NE(old_state, SelectionState::kContain) << layout_object;
    SetShouldInvalidateIfNeeded(layout_object);
  }

  // Invalidate Selection start/end is moving on a same node.
  const SelectionPaintRange& new_paint_range = new_range.PaintRange();
  if (new_paint_range.IsNull() || old_range.IsNull())
    return;
  if (new_paint_range.StartLayoutObject()->IsText() &&
      new_paint_range.StartLayoutObject() == old_range.StartLayoutObject() &&
      new_paint_range.StartOffset() != old_range.StartOffset())
    SetShouldInvalidateIfNeeded(new_paint_range.StartLayoutObject());
  if (new_paint_range.EndLayoutObject()->IsText() &&
      new_paint_range.EndLayoutObject() == old_range.EndLayoutObject() &&
      new_paint_range.EndOffset() != old_range.EndOffset())
    SetShouldInvalidateIfNeeded(new_paint_range.EndLayoutObject());
}

base::Optional<unsigned> LayoutSelection::SelectionStart() const {
  DCHECK(!HasPendingSelection());
  if (paint_range_.IsNull())
    return base::nullopt;
  return paint_range_.StartOffset();
}

base::Optional<unsigned> LayoutSelection::SelectionEnd() const {
  DCHECK(!HasPendingSelection());
  if (paint_range_.IsNull())
    return base::nullopt;
  return paint_range_.EndOffset();
}

static OldSelectedLayoutObjects ResetOldSelectedLayoutObjects(
    const SelectionPaintRange& old_range) {
  OldSelectedLayoutObjects old_selected_objects;
  HashSet<LayoutObject*> containing_block_set;
  for (LayoutObject* layout_object : old_range) {
    const SelectionState old_state = layout_object->GetSelectionState();
    if (old_state == SelectionState::kNone)
      continue;
    if (old_state != SelectionState::kContain)
      old_selected_objects.insert(layout_object, old_state);
    layout_object->SetSelectionState(SelectionState::kNone);

    // Reset containing block SelectionState for CSS ::selection style.
    // See LayoutObject::InvalidatePaintForSelection().
    for (LayoutObject* containing_block = layout_object->ContainingBlock();
         containing_block;
         containing_block = containing_block->ContainingBlock()) {
      if (containing_block_set.Contains(containing_block))
        break;
      containing_block->SetSelectionState(SelectionState::kNone);
      containing_block_set.insert(containing_block);
    }
  }
  return old_selected_objects;
}

void LayoutSelection::ClearSelection() {
  // For querying Layer::compositingState()
  // This is correct, since destroying layout objects needs to cause eager paint
  // invalidations.
  DisableCompositingQueryAsserts disabler;

  // Just return if the selection is already empty.
  if (paint_range_.IsNull())
    return;

  const OldSelectedLayoutObjects& old_selected_objects =
      ResetOldSelectedLayoutObjects(paint_range_);
  for (LayoutObject* const layout_object : old_selected_objects.Keys())
    SetShouldInvalidateIfNeeded(layout_object);

  // Reset selection.
  paint_range_ = SelectionPaintRange();
}

static base::Optional<unsigned> ComputeStartOffset(
    const LayoutObject& layout_object,
    const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return base::nullopt;

  if (layout_node == position.AnchorNode())
    return position.OffsetInContainerNode();
  return 0;
}

static base::Optional<unsigned> ComputeEndOffset(
    const LayoutObject& layout_object,
    const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return base::nullopt;

  if (layout_node == position.AnchorNode())
    return position.OffsetInContainerNode();
  return ToText(layout_node)->length();
}

static LayoutTextFragment* FirstLetterPartFor(LayoutObject* layout_object) {
  if (!layout_object->IsText())
    return nullptr;
  if (!ToLayoutText(layout_object)->IsTextFragment())
    return nullptr;
  return ToLayoutTextFragment(const_cast<LayoutObject*>(
      AssociatedLayoutObjectOf(*layout_object->GetNode(), 0)));
}

static void MarkSelected(SelectedLayoutObjects* selected_objects,
                         LayoutObject* layout_object,
                         SelectionState state) {
  DCHECK(layout_object->CanBeSelectionLeaf());
  SetSelectionStateIfNeeded(layout_object, state);
  selected_objects->insert(layout_object);
}

static void MarkSelectedInside(SelectedLayoutObjects* selected_objects,
                               LayoutObject* layout_object) {
  MarkSelected(selected_objects, layout_object, SelectionState::kInside);
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part)
    return;
  MarkSelected(selected_objects, first_letter_part, SelectionState::kInside);
}

static NewPaintRangeAndSelectedLayoutObjects MarkStartAndEndInOneNode(
    SelectedLayoutObjects selected_objects,
    LayoutObject* layout_object,
    base::Optional<unsigned> start_offset,
    base::Optional<unsigned> end_offset) {
  if (!layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    DCHECK(!end_offset.has_value());
    MarkSelected(&selected_objects, layout_object,
                 SelectionState::kStartAndEnd);
    return {{layout_object, base::nullopt, layout_object, base::nullopt},
            std::move(selected_objects)};
  }

  DCHECK(start_offset.has_value());
  DCHECK(end_offset.has_value());
  DCHECK_GE(end_offset.value(), start_offset.value());
  if (start_offset.value() == end_offset.value())
    return {};
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part) {
    MarkSelected(&selected_objects, layout_object,
                 SelectionState::kStartAndEnd);
    return {{layout_object, start_offset, layout_object, end_offset},
            std::move(selected_objects)};
  }
  const unsigned unsigned_start = start_offset.value();
  const unsigned unsigned_end = end_offset.value();
  LayoutTextFragment* const remaining_part =
      ToLayoutTextFragment(layout_object);
  if (unsigned_start >= remaining_part->Start()) {
    // Case 1: The selection starts and ends in remaining part.
    DCHECK_GT(unsigned_end, remaining_part->Start());
    MarkSelected(&selected_objects, remaining_part,
                 SelectionState::kStartAndEnd);
    return {{remaining_part, unsigned_start - remaining_part->Start(),
             remaining_part, unsigned_end - remaining_part->Start()},
            std::move(selected_objects)};
  }
  if (unsigned_end <= remaining_part->Start()) {
    // Case 2: The selection starts and ends in first letter part.
    MarkSelected(&selected_objects, first_letter_part,
                 SelectionState::kStartAndEnd);
    return {{first_letter_part, start_offset, first_letter_part, end_offset},
            std::move(selected_objects)};
  }
  // Case 3: The selection starts in first-letter part and ends in remaining
  // part.
  DCHECK_GT(unsigned_end, remaining_part->Start());
  MarkSelected(&selected_objects, first_letter_part, SelectionState::kStart);
  MarkSelected(&selected_objects, remaining_part, SelectionState::kEnd);
  return {{first_letter_part, start_offset, remaining_part,
           unsigned_end - remaining_part->Start()},
          std::move(selected_objects)};
}

// LayoutObjectAndOffset represents start or end of SelectionPaintRange.
struct LayoutObjectAndOffset {
  STACK_ALLOCATED();
  LayoutObject* layout_object;
  base::Optional<unsigned> offset;

  explicit LayoutObjectAndOffset(LayoutObject* passed_layout_object)
      : layout_object(passed_layout_object), offset(base::nullopt) {
    DCHECK(passed_layout_object);
    DCHECK(!passed_layout_object->GetNode()->IsTextNode());
  }
  LayoutObjectAndOffset(LayoutText* layout_text, unsigned passed_offset)
      : layout_object(layout_text), offset(passed_offset) {
    DCHECK(layout_object);
  }
};

LayoutObjectAndOffset MarkStart(SelectedLayoutObjects* selected_objects,
                                LayoutObject* start_layout_object,
                                base::Optional<unsigned> start_offset) {
  if (!start_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    MarkSelected(selected_objects, start_layout_object, SelectionState::kStart);
    return LayoutObjectAndOffset(start_layout_object);
  }

  DCHECK(start_offset.has_value());
  const unsigned unsigned_offset = start_offset.value();
  LayoutText* const start_layout_text = ToLayoutText(start_layout_object);
  if (unsigned_offset >= start_layout_text->TextStartOffset()) {
    // |start_offset| is within |start_layout_object| whether it has first
    // letter part or not.
    MarkSelected(selected_objects, start_layout_object, SelectionState::kStart);
    return {start_layout_text,
            unsigned_offset - start_layout_text->TextStartOffset()};
  }

  // |start_layout_object| has first letter part and |start_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(start_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(selected_objects, first_letter_part, SelectionState::kStart);
  MarkSelected(selected_objects, start_layout_text, SelectionState::kInside);
  return {first_letter_part, start_offset.value()};
}

LayoutObjectAndOffset MarkEnd(SelectedLayoutObjects* selected_objects,
                              LayoutObject* end_layout_object,
                              base::Optional<unsigned> end_offset) {
  if (!end_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!end_offset.has_value());
    MarkSelected(selected_objects, end_layout_object, SelectionState::kEnd);
    return LayoutObjectAndOffset(end_layout_object);
  }

  DCHECK(end_offset.has_value());
  const unsigned unsigned_offset = end_offset.value();
  LayoutText* const end_layout_text = ToLayoutText(end_layout_object);
  if (unsigned_offset >= end_layout_text->TextStartOffset()) {
    // |end_offset| is within |end_layout_object| whether it has first
    // letter part or not.
    MarkSelected(selected_objects, end_layout_object, SelectionState::kEnd);
    if (LayoutTextFragment* const first_letter_part =
            FirstLetterPartFor(end_layout_object)) {
      MarkSelected(selected_objects, first_letter_part,
                   SelectionState::kInside);
    }
    return {end_layout_text,
            unsigned_offset - end_layout_text->TextStartOffset()};
  }

  // |end_layout_object| has first letter part and |end_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(end_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(selected_objects, first_letter_part, SelectionState::kEnd);
  return {first_letter_part, end_offset.value()};
}

static NewPaintRangeAndSelectedLayoutObjects MarkStartAndEndInTwoNodes(
    SelectedLayoutObjects selected_objects,
    LayoutObject* start_layout_object,
    base::Optional<unsigned> start_offset,
    LayoutObject* end_layout_object,
    base::Optional<unsigned> end_offset) {
  const LayoutObjectAndOffset& start =
      MarkStart(&selected_objects, start_layout_object, start_offset);
  const LayoutObjectAndOffset& end =
      MarkEnd(&selected_objects, end_layout_object, end_offset);
  return {{start.layout_object, start.offset, end.layout_object, end.offset},
          std::move(selected_objects)};
}

static base::Optional<unsigned> GetTextContentOffset(
    LayoutObject* layout_object,
    base::Optional<unsigned> node_offset) {
  DCHECK(layout_object->EnclosingNGBlockFlow());
  // |layout_object| is start or end of selection and offset is only valid
  // if it is LayoutText.
  if (!layout_object->IsText())
    return base::nullopt;
  // There are LayoutText that selection can't be inside it(BR, WBR,
  // LayoutCounter).
  if (!node_offset.has_value())
    return base::nullopt;
  const Position position_in_dom(*layout_object->GetNode(),
                                 node_offset.value());
  const NGOffsetMapping* const offset_mapping =
      NGOffsetMapping::GetFor(position_in_dom);
  DCHECK(offset_mapping);
  const base::Optional<unsigned>& ng_offset =
      offset_mapping->GetTextContentOffset(position_in_dom);
  return ng_offset;
}

static NewPaintRangeAndSelectedLayoutObjects ComputeNewPaintRange(
    const NewPaintRangeAndSelectedLayoutObjects& new_range,
    LayoutObject* start_layout_object,
    base::Optional<unsigned> start_node_offset,
    LayoutObject* end_layout_object,
    base::Optional<unsigned> end_node_offset) {
  if (new_range.PaintRange().IsNull())
    return {};
  LayoutObject* const start = new_range.PaintRange().StartLayoutObject();
  // If LayoutObject is not in NG, use legacy offset.
  const base::Optional<unsigned> start_offset =
      start->EnclosingNGBlockFlow()
          ? GetTextContentOffset(start_layout_object, start_node_offset)
          : new_range.PaintRange().StartOffset();

  LayoutObject* const end = new_range.PaintRange().EndLayoutObject();
  const base::Optional<unsigned> end_offset =
      end->EnclosingNGBlockFlow()
          ? GetTextContentOffset(end_layout_object, end_node_offset)
          : new_range.PaintRange().EndOffset();

  return {{start, start_offset, end, end_offset},
          std::move(new_range.LayoutObjects())};
}

// ClampOffset modifies |offset| fixed in a range of |text_fragment| start/end
// offsets.
static unsigned ClampOffset(unsigned offset,
                            const NGPhysicalTextFragment& text_fragment) {
  return std::min(std::max(offset, text_fragment.StartOffset()),
                  text_fragment.EndOffset());
}

static bool IsBeforeLineBreak(const NGPaintFragment& fragment) {
  // TODO(yoichio): InlineBlock should not be container line box.
  // See paint/selection/text-selection-inline-block.html.
  const NGPaintFragment* container_line_box = fragment.ContainerLineBox();
  DCHECK(container_line_box);
  const NGPhysicalLineBoxFragment& physical_line_box =
      ToNGPhysicalLineBoxFragment(container_line_box->PhysicalFragment());
  const NGPhysicalFragment* last_leaf_not_linebreak =
      physical_line_box.LastLogicalLeafIgnoringLineBreak();
  DCHECK(last_leaf_not_linebreak);
  if (&fragment.PhysicalFragment() != last_leaf_not_linebreak)
    return false;
  // Even If |fragment| is before linebreak, if its direction differs to line
  // direction, we don't paint line break. See
  // paint/selection/text-selection-newline-mixed-ltr-rtl.html.
  const ShapeResult* shape_result =
      ToNGPhysicalTextFragment(fragment.PhysicalFragment()).TextShapeResult();
  return physical_line_box.BaseDirection() == shape_result->Direction();
}

// FrameSelection holds selection offsets in layout block flow at
// LayoutSelection::Commit() if selection starts/ends within Text that
// each LayoutObject::SelectionState indicates.
// These offset can be out of |text_fragment| because SelectionState is of each
// LayoutText and not of each NGPhysicalTextFragment for it.
LayoutSelectionStatus LayoutSelection::ComputeSelectionStatus(
    const NGPaintFragment& fragment) const {
  const NGPhysicalTextFragment& text_fragment =
      ToNGPhysicalTextFragmentOrDie(fragment.PhysicalFragment());
  // For BR, WBR, no selection painting.
  if (fragment.GetNode() && !fragment.GetNode()->IsTextNode())
    return {0, 0, SelectLineBreak::kNotSelected};
  if (text_fragment.IsLineBreak())
    return {0, 0, SelectLineBreak::kNotSelected};

  switch (text_fragment.GetLayoutObject()->GetSelectionState()) {
    case SelectionState::kStart: {
      DCHECK(SelectionStart().has_value());
      const unsigned start_in_block = SelectionStart().value_or(0);
      const bool is_continuous = start_in_block <= text_fragment.EndOffset();
      return {ClampOffset(start_in_block, text_fragment),
              text_fragment.EndOffset(),
              (is_continuous && IsBeforeLineBreak(fragment))
                  ? SelectLineBreak::kSelected
                  : SelectLineBreak::kNotSelected};
    }
    case SelectionState::kEnd: {
      DCHECK(SelectionEnd().has_value());
      const unsigned end_in_block =
          SelectionEnd().value_or(text_fragment.EndOffset());
      const unsigned end_in_fragment = ClampOffset(end_in_block, text_fragment);
      const bool is_continuous = text_fragment.EndOffset() < end_in_block;
      return {text_fragment.StartOffset(), end_in_fragment,
              (is_continuous && IsBeforeLineBreak(fragment))
                  ? SelectLineBreak::kSelected
                  : SelectLineBreak::kNotSelected};
    }
    case SelectionState::kStartAndEnd: {
      DCHECK(SelectionStart().has_value());
      DCHECK(SelectionEnd().has_value());
      const unsigned start_in_block = SelectionStart().value_or(0);
      const unsigned end_in_block =
          SelectionEnd().value_or(text_fragment.EndOffset());
      const unsigned end_in_fragment = ClampOffset(end_in_block, text_fragment);
      const bool is_continuous = start_in_block <= text_fragment.EndOffset() &&
                                 text_fragment.EndOffset() < end_in_block;
      return {ClampOffset(start_in_block, text_fragment), end_in_fragment,
              (is_continuous && IsBeforeLineBreak(fragment))
                  ? SelectLineBreak::kSelected
                  : SelectLineBreak::kNotSelected};
    }
    case SelectionState::kInside: {
      return {text_fragment.StartOffset(), text_fragment.EndOffset(),
              IsBeforeLineBreak(fragment) ? SelectLineBreak::kSelected
                                          : SelectLineBreak::kNotSelected};
    }
    default:
      // This block is not included in selection.
      return {0, 0, SelectLineBreak::kNotSelected};
  }
}

static NewPaintRangeAndSelectedLayoutObjects
CalcSelectionRangeAndSetSelectionState(const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsNone())
    return {};

  const EphemeralRangeInFlatTree& selection =
      CalcSelectionInFlatTree(frame_selection);
  if (selection.IsCollapsed() || frame_selection.IsHidden())
    return {};

  // Find first/last visible LayoutObject while
  // marking SelectionState and collecting invalidation candidate LayoutObjects.
  LayoutObject* start_layout_object = nullptr;
  LayoutObject* end_layout_object = nullptr;
  SelectedLayoutObjects selected_objects;
  for (const Node& node : selection.Nodes()) {
    LayoutObject* const layout_object = node.GetLayoutObject();
    if (!layout_object || !layout_object->CanBeSelectionLeaf())
      continue;

    if (!start_layout_object) {
      DCHECK(!end_layout_object);
      start_layout_object = end_layout_object = layout_object;
      continue;
    }

    // In this loop, |end_layout_object| is pointing current last candidate
    // LayoutObject and if it is not start and we find next, we mark the
    // current one as kInside.
    if (end_layout_object != start_layout_object)
      MarkSelectedInside(&selected_objects, end_layout_object);
    end_layout_object = layout_object;
  }

  // No valid LayOutObject found.
  if (!start_layout_object) {
    DCHECK(!end_layout_object);
    return {};
  }

  // Compute offset. It has value iff start/end is text.
  const base::Optional<unsigned> start_offset = ComputeStartOffset(
      *start_layout_object, selection.StartPosition().ToOffsetInAnchor());
  const base::Optional<unsigned> end_offset = ComputeEndOffset(
      *end_layout_object, selection.EndPosition().ToOffsetInAnchor());

  NewPaintRangeAndSelectedLayoutObjects new_range =
      start_layout_object == end_layout_object
          ? MarkStartAndEndInOneNode(std::move(selected_objects),
                                     start_layout_object, start_offset,
                                     end_offset)
          : MarkStartAndEndInTwoNodes(std::move(selected_objects),
                                      start_layout_object, start_offset,
                                      end_layout_object, end_offset);

  if (!RuntimeEnabledFeatures::LayoutNGEnabled())
    return new_range;
  return ComputeNewPaintRange(new_range, start_layout_object, start_offset,
                              end_layout_object, end_offset);
}

void LayoutSelection::SetHasPendingSelection() {
  has_pending_selection_ = true;
}

void LayoutSelection::Commit() {
  if (!HasPendingSelection())
    return;
  has_pending_selection_ = false;

  DCHECK(!frame_selection_->GetDocument().NeedsLayoutTreeUpdate());
  DCHECK_GE(frame_selection_->GetDocument().Lifecycle().GetState(),
            DocumentLifecycle::kLayoutClean);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      frame_selection_->GetDocument().Lifecycle());

  const OldSelectedLayoutObjects& old_selected_objects =
      ResetOldSelectedLayoutObjects(paint_range_);
  const NewPaintRangeAndSelectedLayoutObjects& new_range =
      CalcSelectionRangeAndSetSelectionState(*frame_selection_);
  DCHECK(frame_selection_->GetDocument().GetLayoutView()->GetFrameView());
  SetShouldInvalidateSelection(new_range, paint_range_, old_selected_objects);

  paint_range_ = new_range.PaintRange();
  if (paint_range_.IsNull())
    return;
  // TODO(yoichio): Remove this if state.
  // This SelectionState reassignment is ad-hoc patch for
  // prohibiting use-after-free(crbug.com/752715).
  // LayoutText::setSelectionState(state) propergates |state| to ancestor
  // LayoutObjects, which can accidentally change start/end LayoutObject state
  // then LayoutObject::IsSelectionBorder() returns false although we should
  // clear selection at LayoutObject::WillBeRemoved().
  // We should make LayoutObject::setSelectionState() trivial and remove
  // such propagation or at least do it in LayoutSelection.
  if ((paint_range_.StartLayoutObject()->GetSelectionState() !=
           SelectionState::kStart &&
       paint_range_.StartLayoutObject()->GetSelectionState() !=
           SelectionState::kStartAndEnd) ||
      (paint_range_.EndLayoutObject()->GetSelectionState() !=
           SelectionState::kEnd &&
       paint_range_.EndLayoutObject()->GetSelectionState() !=
           SelectionState::kStartAndEnd)) {
    if (paint_range_.StartLayoutObject() == paint_range_.EndLayoutObject()) {
      paint_range_.StartLayoutObject()->SetSelectionState(
          SelectionState::kStartAndEnd);
    } else {
      paint_range_.StartLayoutObject()->SetSelectionState(
          SelectionState::kStart);
      paint_range_.EndLayoutObject()->SetSelectionState(SelectionState::kEnd);
    }
  }
  // TODO(yoichio): If start == end, they should be kStartAndEnd.
  // If not, start.SelectionState == kStart and vice versa.
  DCHECK(paint_range_.StartLayoutObject()->GetSelectionState() ==
             SelectionState::kStart ||
         paint_range_.StartLayoutObject()->GetSelectionState() ==
             SelectionState::kStartAndEnd);
  DCHECK(paint_range_.EndLayoutObject()->GetSelectionState() ==
             SelectionState::kEnd ||
         paint_range_.EndLayoutObject()->GetSelectionState() ==
             SelectionState::kStartAndEnd);
}

void LayoutSelection::OnDocumentShutdown() {
  has_pending_selection_ = false;
  paint_range_ = SelectionPaintRange();
}

static LayoutRect SelectionRectForLayoutObject(const LayoutObject* object) {
  if (!object->IsRooted())
    return LayoutRect();

  if (!object->CanUpdateSelectionOnRootLineBoxes())
    return LayoutRect();

  return object->AbsoluteSelectionRect();
}

IntRect LayoutSelection::AbsoluteSelectionBounds() {
  Commit();
  if (paint_range_.IsNull())
    return IntRect();

  // Create a single bounding box rect that encloses the whole selection.
  LayoutRect selected_rect;
  for (LayoutObject* layout_object : paint_range_) {
    const SelectionState state = layout_object->GetSelectionState();
    if (state == SelectionState::kContain || state == SelectionState::kNone)
      continue;
    selected_rect.Unite(SelectionRectForLayoutObject(layout_object));
  }

  return PixelSnappedIntRect(selected_rect);
}

void LayoutSelection::InvalidatePaintForSelection() {
  if (paint_range_.IsNull())
    return;

  for (LayoutObject* runner : paint_range_) {
    if (runner->GetSelectionState() == SelectionState::kNone)
      continue;

    runner->SetShouldInvalidateSelection();
  }
}

void LayoutSelection::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_selection_);
}

void PrintLayoutObjectForSelection(std::ostream& ostream,
                                   LayoutObject* layout_object) {
  if (!layout_object) {
    ostream << "<null>";
    return;
  }
  ostream << (void*)layout_object << ' ' << layout_object->GetNode()
          << ", state:" << layout_object->GetSelectionState()
          << (layout_object->ShouldInvalidateSelection() ? ", ShouldInvalidate"
                                                         : ", NotInvalidate");
}
#ifndef NDEBUG
void ShowLayoutObjectForSelection(LayoutObject* layout_object) {
  std::stringstream stream;
  PrintLayoutObjectForSelection(stream, layout_object);
  LOG(INFO) << '\n' << stream.str();
}
#endif

}  // namespace blink
