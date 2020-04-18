/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.
 *               All rights reserved.
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
 *
 */

#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_box.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_br.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_ruby_run.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_ruby_text.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/line/abstract_inline_text_box.h"
#include "third_party/blink/renderer/core/layout/line/ellipsis_box.h"
#include "third_party/blink/renderer/core/paint/inline_text_box_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/fonts/character_range.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

#include <algorithm>

namespace blink {

struct SameSizeAsInlineTextBox : public InlineBox {
  unsigned variables[1];
  unsigned short variables2[2];
  void* pointers[2];
};

static_assert(sizeof(InlineTextBox) == sizeof(SameSizeAsInlineTextBox),
              "InlineTextBox should stay small");

typedef WTF::HashMap<const InlineTextBox*, LayoutRect> InlineTextBoxOverflowMap;
static InlineTextBoxOverflowMap* g_text_boxes_with_overflow;

void InlineTextBox::Destroy() {
  AbstractInlineTextBox::WillDestroy(this);

  if (!KnownToHaveNoOverflow() && g_text_boxes_with_overflow)
    g_text_boxes_with_overflow->erase(this);
  InlineBox::Destroy();
}

void InlineTextBox::OffsetRun(int delta) {
  DCHECK(!IsDirty());
  start_ += delta;
}

void InlineTextBox::MarkDirty() {
  len_ = 0;
  start_ = 0;
  InlineBox::MarkDirty();
}

LayoutRect InlineTextBox::LogicalOverflowRect() const {
  if (KnownToHaveNoOverflow() || !g_text_boxes_with_overflow)
    return LogicalFrameRect();

  const auto& it = g_text_boxes_with_overflow->find(this);
  if (it != g_text_boxes_with_overflow->end())
    return it->value;

  return LogicalFrameRect();
}

void InlineTextBox::SetLogicalOverflowRect(const LayoutRect& rect) {
  DCHECK(!KnownToHaveNoOverflow());
  DCHECK(rect != LogicalFrameRect());
  if (!g_text_boxes_with_overflow)
    g_text_boxes_with_overflow = new InlineTextBoxOverflowMap;
  g_text_boxes_with_overflow->Set(this, rect);
}

void InlineTextBox::Move(const LayoutSize& delta) {
  InlineBox::Move(delta);

  if (!KnownToHaveNoOverflow() && g_text_boxes_with_overflow) {
    const auto& it = g_text_boxes_with_overflow->find(this);
    if (it != g_text_boxes_with_overflow->end())
      it->value.Move(IsHorizontal() ? delta : delta.TransposedSize());
  }
}

LayoutUnit InlineTextBox::BaselinePosition(FontBaseline baseline_type) const {
  if (!IsText() || !Parent())
    return LayoutUnit();
  if (Parent()->GetLineLayoutItem() == GetLineLayoutItem().Parent())
    return Parent()->BaselinePosition(baseline_type);
  return LineLayoutBoxModel(GetLineLayoutItem().Parent())
      .BaselinePosition(baseline_type, IsFirstLineStyle(),
                        IsHorizontal() ? kHorizontalLine : kVerticalLine,
                        kPositionOnContainingLine);
}

LayoutUnit InlineTextBox::LineHeight() const {
  if (!IsText() || !GetLineLayoutItem().Parent())
    return LayoutUnit();
  if (GetLineLayoutItem().IsBR()) {
    return LayoutUnit(
        LineLayoutBR(GetLineLayoutItem()).LineHeight(IsFirstLineStyle()));
  }
  if (Parent()->GetLineLayoutItem() == GetLineLayoutItem().Parent())
    return Parent()->LineHeight();
  return LineLayoutBoxModel(GetLineLayoutItem().Parent())
      .LineHeight(IsFirstLineStyle(),
                  IsHorizontal() ? kHorizontalLine : kVerticalLine,
                  kPositionOnContainingLine);
}

LayoutUnit InlineTextBox::OffsetTo(FontVerticalPositionType position_type,
                                   FontBaseline baseline_type) const {
  if (IsText() &&
      (position_type == FontVerticalPositionType::TopOfEmHeight ||
       position_type == FontVerticalPositionType::BottomOfEmHeight)) {
    const Font& font = GetLineLayoutItem().Style(IsFirstLineStyle())->GetFont();
    if (const SimpleFontData* font_data = font.PrimaryFont()) {
      return font_data->GetFontMetrics().Ascent(baseline_type) -
             font_data->VerticalPosition(position_type, baseline_type);
    }
  }
  switch (position_type) {
    case FontVerticalPositionType::TextTop:
    case FontVerticalPositionType::TopOfEmHeight:
      return LayoutUnit();
    case FontVerticalPositionType::TextBottom:
    case FontVerticalPositionType::BottomOfEmHeight:
      return LogicalHeight();
  }
  NOTREACHED();
  return LayoutUnit();
}

LayoutUnit InlineTextBox::VerticalPosition(
    FontVerticalPositionType position_type,
    FontBaseline baseline_type) const {
  return LogicalTop() + OffsetTo(position_type, baseline_type);
}

bool InlineTextBox::IsSelected(int start_pos, int end_pos) const {
  int s_pos = std::max(start_pos - start_, 0);
  // The position after a hard line break is considered to be past its end.
  // See the corresponding code in InlineTextBox::getSelectionState.
  int e_pos = std::min(end_pos - start_, int(len_) + (IsLineBreak() ? 0 : 1));
  return (s_pos < e_pos);
}

SelectionState InlineTextBox::GetSelectionState() const {
  SelectionState state = GetLineLayoutItem().GetSelectionState();
  if (state == SelectionState::kStart || state == SelectionState::kEnd ||
      state == SelectionState::kStartAndEnd) {
    // The position after a hard line break is considered to be past its end.
    // See the corresponding code in InlineTextBox::isSelected.
    int last_selectable = Start() + Len() - (IsLineBreak() ? 1 : 0);

    // FIXME: Remove -webkit-line-break: LineBreakAfterWhiteSpace.
    int end_of_line_adjustment_for_css_line_break =
        GetLineLayoutItem().Style()->GetLineBreak() ==
                LineBreak::kAfterWhiteSpace
            ? -1
            : 0;
    const FrameSelection& selection =
        GetLineLayoutItem().GetDocument().GetFrame()->Selection();
    // TODO(yoichio): |value_or()| is used to prevent use uininitialized
    // value on release build. It should be value() because calling
    // LayoutSelectionStart() if SelectionState is neigher kStart nor
    // kStartAndEnd is invalid operation.
    bool start =
        (state != SelectionState::kEnd &&
         static_cast<int>(selection.LayoutSelectionStart().value_or(0)) >=
             start_ &&
         static_cast<int>(selection.LayoutSelectionStart().value_or(0)) <=
             start_ + len_ + end_of_line_adjustment_for_css_line_break);
    bool end = (state != SelectionState::kStart &&
                static_cast<int>(selection.LayoutSelectionEnd().value_or(0)) >
                    start_ &&
                static_cast<int>(selection.LayoutSelectionEnd().value_or(0)) <=
                    last_selectable);
    if (start && end)
      state = SelectionState::kStartAndEnd;
    else if (start)
      state = SelectionState::kStart;
    else if (end)
      state = SelectionState::kEnd;
    else if ((state == SelectionState::kEnd ||
              static_cast<int>(selection.LayoutSelectionStart().value_or(0)) <
                  start_) &&
             (state == SelectionState::kStart ||
              static_cast<int>(selection.LayoutSelectionEnd().value_or(0)) >
                  last_selectable))
      state = SelectionState::kInside;
    else if (state == SelectionState::kStartAndEnd)
      state = SelectionState::kNone;
  }

  // If there are ellipsis following, make sure their selection is updated.
  if (EllipsisBox* ellipsis = Root().GetEllipsisBox()) {
    if (state != SelectionState::kNone) {
      int start, end;
      SelectionStartEnd(start, end);
      // The ellipsis should be considered to be selected if the end of the
      // selection is past the beginning of the truncation and the beginning of
      // the selection is before or at the beginning of the truncation.
      ellipsis->SetSelectionState(end >= truncation_ && start <= truncation_
                                      ? SelectionState::kInside
                                      : SelectionState::kNone);
    } else {
      ellipsis->SetSelectionState(SelectionState::kNone);
    }
  }

  return state;
}

bool InlineTextBox::HasWrappedSelectionNewline() const {
  DCHECK(!GetLineLayoutItem().NeedsLayout());

  SelectionState state = GetSelectionState();
  if (state != SelectionState::kStart && state != SelectionState::kInside)
    return false;

  // Checking last leaf child can be slow, so we make sure to do this
  // only after checking selection state.
  if (Root().LastLeafChild() != this)
    return false;

  // It's possible to have mixed LTR/RTL on a single line, and we only
  // want to paint a newline when we're the last leaf child and we make
  // sure there isn't a differently-directioned box following us.
  bool is_ltr = IsLeftToRightDirection();
  if ((!is_ltr && Root().FirstSelectedBox() != this) ||
      (is_ltr && Root().LastSelectedBox() != this))
    return false;

  // If we're the last inline text box in containing block, our containing block
  // is inline, and the selection continues into that block, then rely on the
  // next inline text box (if any) to paint a wrapped new line as needed.
  if (NextForSameLayoutObject())
    return true;
  auto root_block = Root().Block();
  if (root_block.IsInline() &&
      root_block.GetSelectionState() != SelectionState::kEnd &&
      root_block.GetSelectionState() != SelectionState::kStartAndEnd &&
      root_block.InlineBoxWrapper() &&
      ((is_ltr && root_block.InlineBoxWrapper()->NextOnLine()) ||
       (!is_ltr && root_block.InlineBoxWrapper()->PrevOnLine()))) {
    return false;
  }

  return true;
}

float InlineTextBox::NewlineSpaceWidth() const {
  const ComputedStyle& style_to_use =
      GetLineLayoutItem().StyleRef(IsFirstLineStyle());
  return style_to_use.GetFont().SpaceWidth();
}

LayoutRect InlineTextBox::LocalSelectionRect(
    int start_pos,
    int end_pos,
    bool consider_current_selection) const {
  int s_pos = std::max(start_pos - start_, 0);
  int e_pos = std::min(end_pos - start_, (int)len_);

  if (s_pos > e_pos)
    return LayoutRect();

  FontCachePurgePreventer font_cache_purge_preventer;

  LayoutUnit sel_top = Root().SelectionTop();
  LayoutUnit sel_height = Root().SelectionHeight();
  const ComputedStyle& style_to_use =
      GetLineLayoutItem().StyleRef(IsFirstLineStyle());
  const Font& font = style_to_use.GetFont();

  StringBuilder characters_with_hyphen;
  bool respect_hyphen = e_pos == len_ && HasHyphen();
  TextRun text_run = ConstructTextRun(
      style_to_use, respect_hyphen ? &characters_with_hyphen : nullptr);

  LayoutPoint starting_point = LayoutPoint(LogicalLeft(), sel_top);
  LayoutRect r;
  if (s_pos || e_pos != static_cast<int>(len_)) {
    r = LayoutRect(EnclosingIntRect(
        font.SelectionRectForText(text_run, FloatPoint(starting_point),
                                  sel_height.ToInt(), s_pos, e_pos)));
  } else {
    // Avoid computing the font width when the entire line box is selected as an
    // optimization.
    r = LayoutRect(EnclosingIntRect(
        LayoutRect(starting_point, LayoutSize(logical_width_, sel_height))));
  }

  LayoutUnit logical_width = r.Width();
  if (r.X() > LogicalRight())
    logical_width = LayoutUnit();
  else if (r.MaxX() > LogicalRight())
    logical_width = LogicalRight() - r.X();

  LayoutPoint top_point;
  LayoutUnit width;
  LayoutUnit height;
  if (IsHorizontal()) {
    top_point = LayoutPoint(r.X(), sel_top);
    width = logical_width;
    height = sel_height;
    if (consider_current_selection && HasWrappedSelectionNewline()) {
      if (!IsLeftToRightDirection())
        top_point.SetX(LayoutUnit(top_point.X() - NewlineSpaceWidth()));
      width += NewlineSpaceWidth();
    }
  } else {
    top_point = LayoutPoint(sel_top, r.X());
    width = sel_height;
    height = logical_width;
    // TODO(wkorman): RTL text embedded in top-to-bottom text can create
    // bottom-to-top situations. Add tests and ensure we handle correctly.
    if (consider_current_selection && HasWrappedSelectionNewline())
      height += NewlineSpaceWidth();
  }

  return LayoutRect(top_point, LayoutSize(width, height));
}

void InlineTextBox::DeleteLine() {
  GetLineLayoutItem().RemoveTextBox(this);
  Destroy();
}

void InlineTextBox::ExtractLine() {
  if (Extracted())
    return;

  GetLineLayoutItem().ExtractTextBox(this);
}

void InlineTextBox::AttachLine() {
  if (!Extracted())
    return;

  GetLineLayoutItem().AttachTextBox(this);
}

void InlineTextBox::SetTruncation(unsigned truncation) {
  if (truncation == truncation_)
    return;

  truncation_ = truncation;
}

void InlineTextBox::ClearTruncation() {
  SetTruncation(kCNoTruncation);
}

LayoutUnit InlineTextBox::PlaceEllipsisBox(bool flow_is_ltr,
                                           LayoutUnit visible_left_edge,
                                           LayoutUnit visible_right_edge,
                                           LayoutUnit ellipsis_width,
                                           LayoutUnit& truncated_width,
                                           InlineBox** found_box,
                                           LayoutUnit logical_left_offset) {
  if (*found_box) {
    SetTruncation(kCFullTruncation);
    return LayoutUnit(-1);
  }

  // Criteria for full truncation:
  // LTR: the left edge of the ellipsis is to the left of our text run.
  // RTL: the right edge of the ellipsis is to the right of our text run.
  LayoutUnit adjusted_logical_left = logical_left_offset + LogicalLeft();

  // For LTR this is the left edge of the box, for RTL, the right edge in parent
  // coordinates.
  LayoutUnit ellipsis_x = flow_is_ltr ? visible_right_edge - ellipsis_width
                                      : visible_left_edge + ellipsis_width;

  bool ltr_full_truncation = flow_is_ltr && ellipsis_x <= adjusted_logical_left;
  bool rtl_full_truncation =
      !flow_is_ltr && ellipsis_x > adjusted_logical_left + LogicalWidth();
  if (ltr_full_truncation || rtl_full_truncation) {
    // Too far.  Just set full truncation, but return -1 and let the ellipsis
    // just be placed at the edge of the box.
    SetTruncation(kCFullTruncation);
    *found_box = this;
    return LayoutUnit(-1);
  }

  bool ltr_ellipsis_within_box =
      flow_is_ltr && ellipsis_x < adjusted_logical_left + LogicalWidth();
  bool rtl_ellipsis_within_box =
      !flow_is_ltr && ellipsis_x > adjusted_logical_left;
  if (ltr_ellipsis_within_box || rtl_ellipsis_within_box) {
    *found_box = this;

    // OffsetForPosition() expects the position relative to the root box.
    ellipsis_x -= logical_left_offset;

    // We measure the text using the second half of the previous character and
    // the first half of the current one when the text is rtl. This gives a
    // more accurate position in rtl text.
    // TODO(crbug.com/722043: This doesn't always give the best results.
    bool ltr = IsLeftToRightDirection();
    int offset = OffsetForPosition(ellipsis_x, !ltr);
    // Full truncation is only necessary when we're flowing left-to-right.
    if (flow_is_ltr && offset == 0 && ltr == flow_is_ltr) {
      // No characters should be laid out.  Set ourselves to full truncation and
      // place the ellipsis at the min of our start and the ellipsis edge.
      SetTruncation(kCFullTruncation);
      truncated_width += ellipsis_width;
      return std::min(ellipsis_x, LogicalLeft());
    }

    // When the text's direction doesn't match the flow's direction we can
    // choose an offset that starts outside the visible box: compensate for that
    // if necessary.
    if (flow_is_ltr != ltr && LogicalLeft() < 0 && offset >= start_ &&
        PositionForOffset(offset) < LogicalLeft().Abs())
      offset++;

    // Set the truncation index on the text run.
    SetTruncation(offset);

    // If we got here that means that we were only partially truncated and we
    // need to return the pixel offset at which to place the ellipsis. Where the
    // text and its flow have opposite directions then our offset into the text
    // is at the start of the part that will be visible.
    LayoutUnit width_of_visible_text(GetLineLayoutItem().Width(
        ltr == flow_is_ltr ? start_ : start_ + offset,
        ltr == flow_is_ltr ? offset : len_ - offset, TextPos(),
        flow_is_ltr ? TextDirection::kLtr : TextDirection::kRtl,
        IsFirstLineStyle(), nullptr, nullptr, Expansion()));

    // The ellipsis needs to be placed just after the last visible character.
    // Where "after" is defined by the flow directionality, not the inline
    // box directionality.
    // e.g. In the case of an LTR inline box truncated in an RTL flow then we
    // can have a situation such as |Hello| -> |...He|
    truncated_width += width_of_visible_text + ellipsis_width;
    if (flow_is_ltr)
      return LogicalLeft() + width_of_visible_text;
    LayoutUnit result = LogicalRight() - width_of_visible_text - ellipsis_width;
    return result;
  }
  truncated_width += LogicalWidth();
  return LayoutUnit(-1);
}

bool InlineTextBox::IsLineBreak() const {
  return GetLineLayoutItem().IsBR() ||
         (GetLineLayoutItem().Style()->PreserveNewline() && Len() == 1 &&
          GetLineLayoutItem().GetText().length() > Start() &&
          (*GetLineLayoutItem().GetText().Impl())[Start()] == '\n');
}

bool InlineTextBox::NodeAtPoint(HitTestResult& result,
                                const HitTestLocation& location_in_container,
                                const LayoutPoint& accumulated_offset,
                                LayoutUnit /* lineTop */,
                                LayoutUnit /*lineBottom*/) {
  if (IsLineBreak() || truncation_ == kCFullTruncation)
    return false;

  LayoutPoint box_origin = PhysicalLocation();
  box_origin.MoveBy(accumulated_offset);
  LayoutRect rect(box_origin, Size());
  if (VisibleToHitTestRequest(result.GetHitTestRequest()) &&
      location_in_container.Intersects(rect)) {
    GetLineLayoutItem().UpdateHitTestResult(
        result, FlipForWritingMode(location_in_container.Point() -
                                   ToLayoutSize(accumulated_offset)));
    if (result.AddNodeToListBasedTestResult(GetLineLayoutItem().GetNode(),
                                            location_in_container,
                                            rect) == kStopHitTesting)
      return true;
  }
  return false;
}

bool InlineTextBox::GetEmphasisMarkPosition(
    const ComputedStyle& style,
    TextEmphasisPosition& emphasis_position) const {
  // This function returns true if there are text emphasis marks and they are
  // suppressed by ruby text.
  if (style.GetTextEmphasisMark() == TextEmphasisMark::kNone)
    return false;

  emphasis_position = style.GetTextEmphasisPosition();
  // Ruby text is always over, so it cannot suppress emphasis marks under.
  if (style.GetTextEmphasisLineLogicalSide() != LineLogicalSide::kOver)
    return true;

  LineLayoutBox containing_block = GetLineLayoutItem().ContainingBlock();
  // This text is not inside a ruby base, so it does not have ruby text over it.
  if (!containing_block.IsRubyBase())
    return true;

  // Cannot get the ruby text.
  if (!containing_block.Parent().IsRubyRun())
    return true;

  LineLayoutRubyText ruby_text =
      LineLayoutRubyRun(containing_block.Parent()).RubyText();

  // The emphasis marks over are suppressed only if there is a ruby text box and
  // it not empty.
  return !ruby_text || !ruby_text.FirstLineBox();
}

void InlineTextBox::Paint(const PaintInfo& paint_info,
                          const LayoutPoint& paint_offset,
                          LayoutUnit /*lineTop*/,
                          LayoutUnit /*lineBottom*/) const {
  InlineTextBoxPainter(*this).Paint(paint_info, paint_offset);
  if (GetLineLayoutItem().ContainsOnlyWhitespaceOrNbsp() !=
      OnlyWhitespaceOrNbsp::kYes) {
    paint_info.context.GetPaintController().SetTextPainted();
  }
}

void InlineTextBox::SelectionStartEnd(int& s_pos, int& e_pos) const {
  int start_pos, end_pos;
  if (GetLineLayoutItem().GetSelectionState() == SelectionState::kInside) {
    start_pos = 0;
    end_pos = GetLineLayoutItem().TextLength();
  } else {
    const FrameSelection& selection =
        GetLineLayoutItem().GetDocument().GetFrame()->Selection();
    // TODO(yoichio): |value_or()| is used to prevent use uininitialized
    // value on release build. It should be |value()| because calling
    // LayoutSelectionStart() if SelectionState is neigher kStart nor
    // kStartAndEnd is invalid operation.
    if (GetLineLayoutItem().GetSelectionState() == SelectionState::kStart) {
      start_pos = selection.LayoutSelectionStart().value_or(0);
      end_pos = GetLineLayoutItem().TextLength();
    } else if (GetLineLayoutItem().GetSelectionState() ==
               SelectionState::kEnd) {
      start_pos = 0;
      end_pos = selection.LayoutSelectionEnd().value_or(0);
    } else {
      DCHECK(GetLineLayoutItem().GetSelectionState() ==
             SelectionState::kStartAndEnd);
      start_pos = selection.LayoutSelectionStart().value_or(0);
      end_pos = selection.LayoutSelectionEnd().value_or(0);
    }
  }

  s_pos = std::max(start_pos - start_, 0);
  e_pos = std::min(end_pos - start_, (int)len_);
}

void InlineTextBox::PaintDocumentMarker(GraphicsContext& pt,
                                        const LayoutPoint& box_origin,
                                        const DocumentMarker& marker,
                                        const ComputedStyle& style,
                                        const Font& font,
                                        bool grammar) const {
  InlineTextBoxPainter(*this).PaintDocumentMarker(pt, box_origin, marker, style,
                                                  font, grammar);
}

void InlineTextBox::PaintTextMatchMarkerForeground(
    const PaintInfo& paint_info,
    const LayoutPoint& box_origin,
    const TextMatchMarker& marker,
    const ComputedStyle& style,
    const Font& font) const {
  InlineTextBoxPainter(*this).PaintTextMatchMarkerForeground(
      paint_info, box_origin, marker, style, font);
  if (GetLineLayoutItem().ContainsOnlyWhitespaceOrNbsp() !=
      OnlyWhitespaceOrNbsp::kYes) {
    paint_info.context.GetPaintController().SetTextPainted();
  }
}

void InlineTextBox::PaintTextMatchMarkerBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& box_origin,
    const TextMatchMarker& marker,
    const ComputedStyle& style,
    const Font& font) const {
  InlineTextBoxPainter(*this).PaintTextMatchMarkerBackground(
      paint_info, box_origin, marker, style, font);
}

int InlineTextBox::CaretMinOffset() const {
  return start_;
}

int InlineTextBox::CaretMaxOffset() const {
  return start_ + len_;
}

LayoutUnit InlineTextBox::TextPos() const {
  // When computing the width of a text run, LayoutBlock::
  // computeInlineDirectionPositionsForLine() doesn't include the actual offset
  // from the containing block edge in its measurement. textPos() should be
  // consistent so the text are laid out in the same width.
  if (LogicalLeft() == 0)
    return LayoutUnit();
  return LogicalLeft() - Root().LogicalLeft();
}

int InlineTextBox::OffsetForPosition(LayoutUnit line_offset,
                                     bool include_partial_glyphs) const {
  if (IsLineBreak())
    return 0;

  if (line_offset - LogicalLeft() > LogicalWidth())
    return IsLeftToRightDirection() ? Len() : 0;
  if (line_offset - LogicalLeft() < 0)
    return IsLeftToRightDirection() ? 0 : Len();

  LineLayoutText text = GetLineLayoutItem();
  const ComputedStyle& style = text.StyleRef(IsFirstLineStyle());
  const Font& font = style.GetFont();
  return font.OffsetForPosition(ConstructTextRun(style),
                                (line_offset - LogicalLeft()).ToFloat(),
                                include_partial_glyphs);
}

LayoutUnit InlineTextBox::PositionForOffset(int offset) const {
  DCHECK_GE(offset, start_);
  DCHECK_LE(offset, start_ + len_);

  if (IsLineBreak())
    return LogicalLeft();

  LineLayoutText text = GetLineLayoutItem();
  const ComputedStyle& style_to_use = text.StyleRef(IsFirstLineStyle());
  const Font& font = style_to_use.GetFont();
  int from = !IsLeftToRightDirection() ? offset - start_ : 0;
  int to = !IsLeftToRightDirection() ? len_ : offset - start_;
  // FIXME: Do we need to add rightBearing here?
  return LayoutUnit(
      font.SelectionRectForText(ConstructTextRun(style_to_use),
                                IntPoint(LogicalLeft().ToInt(), 0), 0, from, to)
          .MaxX());
}

bool InlineTextBox::ContainsCaretOffset(int offset) const {
  // Offsets before the box are never "in".
  if (offset < start_)
    return false;

  int past_end = start_ + len_;

  // Offsets inside the box (not at either edge) are always "in".
  if (offset < past_end)
    return true;

  // Offsets outside the box are always "out".
  if (offset > past_end)
    return false;

  // Offsets at the end are "out" for line breaks (they are on the next line).
  if (IsLineBreak())
    return false;

  // Offsets at the end are "in" for normal boxes (but the caller has to check
  // affinity).
  return true;
}

void InlineTextBox::CharacterWidths(Vector<float>& widths) const {
  if (!len_)
    return;

  FontCachePurgePreventer font_cache_purge_preventer;
  DCHECK(GetLineLayoutItem().GetText());

  const ComputedStyle& style_to_use =
      GetLineLayoutItem().StyleRef(IsFirstLineStyle());
  const Font& font = style_to_use.GetFont();

  TextRun text_run = ConstructTextRun(style_to_use);
  Vector<CharacterRange> ranges = font.IndividualCharacterRanges(text_run);
  DCHECK_EQ(ranges.size(), len_);

  widths.resize(ranges.size());
  for (unsigned i = 0; i < ranges.size(); i++)
    widths[i] = ranges[i].Width();
}

TextRun InlineTextBox::ConstructTextRun(
    const ComputedStyle& style,
    StringBuilder* characters_with_hyphen) const {
  DCHECK(GetLineLayoutItem().GetText());

  String string = GetLineLayoutItem().GetText();
  unsigned start_pos = Start();
  unsigned length = Len();
  // Ensure |this| is in sync with the corresponding LayoutText. Checking here
  // has less binary size/perf impact than in StringView().
  CHECK_LE(start_pos, string.length());
  CHECK_LE(length, string.length() - start_pos);
  return ConstructTextRun(style, StringView(string, start_pos, length),
                          GetLineLayoutItem().TextLength() - start_pos,
                          characters_with_hyphen);
}

TextRun InlineTextBox::ConstructTextRun(
    const ComputedStyle& style,
    StringView string,
    int maximum_length,
    StringBuilder* characters_with_hyphen) const {
  if (characters_with_hyphen) {
    const AtomicString& hyphen_string = style.HyphenString();
    characters_with_hyphen->ReserveCapacity(string.length() +
                                            hyphen_string.length());
    characters_with_hyphen->Append(string);
    characters_with_hyphen->Append(hyphen_string);
    string = characters_with_hyphen->ToString();
    maximum_length = string.length();
  }

  DCHECK_GE(maximum_length, static_cast<int>(string.length()));

  TextRun run(string, TextPos().ToFloat(), Expansion(), GetExpansionBehavior(),
              Direction(),
              DirOverride() || style.RtlOrdering() == EOrder::kVisual);
  run.SetTabSize(!style.CollapseWhiteSpace(), style.GetTabSize());
  run.SetTextJustify(style.GetTextJustify());

  // Propagate the maximum length of the characters buffer to the TextRun, even
  // when we're only processing a substring.
  run.SetCharactersLength(maximum_length);
  DCHECK_GE(run.CharactersLength(), run.length());
  return run;
}

TextRun InlineTextBox::ConstructTextRunForInspector(
    const ComputedStyle& style) const {
  return InlineTextBox::ConstructTextRun(style);
}

const char* InlineTextBox::BoxName() const {
  return "InlineTextBox";
}

String InlineTextBox::DebugName() const {
  return String(BoxName()) + " '" + GetText() + "'";
}

String InlineTextBox::GetText() const {
  return GetLineLayoutItem().GetText().Substring(Start(), Len());
}

#ifndef NDEBUG

void InlineTextBox::DumpBox(StringBuilder& string_inlinetextbox) const {
  String value = GetText();
  value.Replace('\\', "\\\\");
  value.Replace('\n', "\\n");
  string_inlinetextbox.Append(String::Format("%s %p", BoxName(), this));
  while (string_inlinetextbox.length() < kShowTreeCharacterOffset)
    string_inlinetextbox.Append(" ");
  const LineLayoutText obj = GetLineLayoutItem();
  string_inlinetextbox.Append(
      String::Format("\t%s %p", obj.GetName(), obj.DebugPointer()));
  const int kLayoutObjectCharacterOffset = 75;
  while (string_inlinetextbox.length() < kLayoutObjectCharacterOffset)
    string_inlinetextbox.Append(" ");
  string_inlinetextbox.Append(String::Format(
      "(%d,%d) \"%s\"", Start(), Start() + Len(), value.Utf8().data()));
}

#endif

}  // namespace blink
