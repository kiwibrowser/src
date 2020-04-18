/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/layout/layout_scrollbar_part.h"

#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar_theme.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/platform/length_functions.h"

namespace blink {

LayoutScrollbarPart::LayoutScrollbarPart(ScrollableArea* scrollable_area,
                                         LayoutScrollbar* scrollbar,
                                         ScrollbarPart part)
    : LayoutBlock(nullptr),
      scrollable_area_(scrollable_area),
      scrollbar_(scrollbar),
      part_(part) {
  DCHECK(scrollable_area_);
}

static void RecordScrollbarPartStats(Document& document, ScrollbarPart part) {
  switch (part) {
    case kBackButtonStartPart:
    case kForwardButtonStartPart:
    case kBackButtonEndPart:
    case kForwardButtonEndPart:
      UseCounter::Count(document,
                        WebFeature::kCSSSelectorPseudoScrollbarButton);
      break;
    case kBackTrackPart:
    case kForwardTrackPart:
      UseCounter::Count(document,
                        WebFeature::kCSSSelectorPseudoScrollbarTrackPiece);
      break;
    case kThumbPart:
      UseCounter::Count(document, WebFeature::kCSSSelectorPseudoScrollbarThumb);
      break;
    case kTrackBGPart:
      UseCounter::Count(document, WebFeature::kCSSSelectorPseudoScrollbarTrack);
      break;
    case kScrollbarBGPart:
      UseCounter::Count(document, WebFeature::kCSSSelectorPseudoScrollbar);
      break;
    case kNoPart:
    case kAllParts:
      break;
  }
}

LayoutScrollbarPart* LayoutScrollbarPart::CreateAnonymous(
    Document* document,
    ScrollableArea* scrollable_area,
    LayoutScrollbar* scrollbar,
    ScrollbarPart part) {
  LayoutScrollbarPart* layout_object =
      new LayoutScrollbarPart(scrollable_area, scrollbar, part);
  RecordScrollbarPartStats(*document, part);
  layout_object->SetDocumentForAnonymous(document);
  return layout_object;
}

void LayoutScrollbarPart::UpdateLayout() {
  // We don't worry about positioning ourselves. We're just determining our
  // minimum width/height.
  SetLocation(LayoutPoint());
  if (scrollbar_->Orientation() == kHorizontalScrollbar)
    LayoutHorizontalPart();
  else
    LayoutVerticalPart();

  ClearNeedsLayout();
}

void LayoutScrollbarPart::LayoutHorizontalPart() {
  if (part_ == kScrollbarBGPart) {
    SetWidth(LayoutUnit(scrollbar_->Width()));
    ComputeScrollbarHeight();
  } else {
    ComputeScrollbarWidth();
    SetHeight(LayoutUnit(scrollbar_->Height()));
  }
}

void LayoutScrollbarPart::LayoutVerticalPart() {
  if (part_ == kScrollbarBGPart) {
    ComputeScrollbarWidth();
    SetHeight(LayoutUnit(scrollbar_->Height()));
  } else {
    SetWidth(LayoutUnit(scrollbar_->Width()));
    ComputeScrollbarHeight();
  }
}

int LayoutScrollbarPart::CalcScrollbarThicknessUsing(SizeType size_type,
                                                     const Length& length,
                                                     int containing_length) {
  if (!length.IsIntrinsicOrAuto() || (size_type == kMinSize && length.IsAuto()))
    return MinimumValueForLength(length, LayoutUnit(containing_length)).ToInt();
  return scrollbar_->GetTheme().ScrollbarThickness();
}

void LayoutScrollbarPart::ComputeScrollbarWidth() {
  if (!scrollbar_->StyleSource())
    return;
  // FIXME: We are querying layout information but nothing guarantees that it's
  // up to date, especially since we are called at style change.
  // FIXME: Querying the style's border information doesn't work on table cells
  // with collapsing borders.
  int visible_size = scrollbar_->StyleSource()->Size().Width() -
                     scrollbar_->StyleSource()->Style()->BorderLeftWidth() -
                     scrollbar_->StyleSource()->Style()->BorderRightWidth();
  int w = CalcScrollbarThicknessUsing(kMainOrPreferredSize, Style()->Width(),
                                      visible_size);
  int min_width =
      CalcScrollbarThicknessUsing(kMinSize, Style()->MinWidth(), visible_size);
  int max_width = Style()->MaxWidth().IsMaxSizeNone()
                      ? w
                      : CalcScrollbarThicknessUsing(
                            kMaxSize, Style()->MaxWidth(), visible_size);
  SetWidth(LayoutUnit(std::max(min_width, std::min(max_width, w))));

  // Buttons and track pieces can all have margins along the axis of the
  // scrollbar. Values are rounded because scrollbar parts need to be rendered
  // at device pixel boundaries.
  SetMarginLeft(LayoutUnit(
      MinimumValueForLength(Style()->MarginLeft(), LayoutUnit(visible_size))
          .Round()));
  SetMarginRight(LayoutUnit(
      MinimumValueForLength(Style()->MarginRight(), LayoutUnit(visible_size))
          .Round()));
}

void LayoutScrollbarPart::ComputeScrollbarHeight() {
  if (!scrollbar_->StyleSource())
    return;
  // FIXME: We are querying layout information but nothing guarantees that it's
  // up to date, especially since we are called at style change.
  // FIXME: Querying the style's border information doesn't work on table cells
  // with collapsing borders.
  int visible_size = scrollbar_->StyleSource()->Size().Height() -
                     scrollbar_->StyleSource()->Style()->BorderTopWidth() -
                     scrollbar_->StyleSource()->Style()->BorderBottomWidth();
  int h = CalcScrollbarThicknessUsing(kMainOrPreferredSize, Style()->Height(),
                                      visible_size);
  int min_height =
      CalcScrollbarThicknessUsing(kMinSize, Style()->MinHeight(), visible_size);
  int max_height = Style()->MaxHeight().IsMaxSizeNone()
                       ? h
                       : CalcScrollbarThicknessUsing(
                             kMaxSize, Style()->MaxHeight(), visible_size);
  SetHeight(LayoutUnit(std::max(min_height, std::min(max_height, h))));

  // Buttons and track pieces can all have margins along the axis of the
  // scrollbar. Values are rounded because scrollbar parts need to be rendered
  // at device pixel boundaries.
  SetMarginTop(LayoutUnit(
      MinimumValueForLength(Style()->MarginTop(), LayoutUnit(visible_size))
          .Round()));
  SetMarginBottom(LayoutUnit(
      MinimumValueForLength(Style()->MarginBottom(), LayoutUnit(visible_size))
          .Round()));
}

void LayoutScrollbarPart::ComputePreferredLogicalWidths() {
  if (!PreferredLogicalWidthsDirty())
    return;

  min_preferred_logical_width_ = max_preferred_logical_width_ = LayoutUnit();

  ClearPreferredLogicalWidthsDirty();
}

void LayoutScrollbarPart::StyleWillChange(StyleDifference diff,
                                          const ComputedStyle& new_style) {
  LayoutBlock::StyleWillChange(diff, new_style);
  SetInline(false);
}

void LayoutScrollbarPart::StyleDidChange(StyleDifference diff,
                                         const ComputedStyle* old_style) {
  LayoutBlock::StyleDidChange(diff, old_style);
  // See adjustStyleBeforeSet() above.
  DCHECK(!IsOrthogonalWritingModeRoot());
  SetInline(false);
  ClearPositionedState();
  SetFloating(false);
  if (old_style && (diff.NeedsFullPaintInvalidation() || diff.NeedsLayout()))
    SetNeedsPaintInvalidation();
}

void LayoutScrollbarPart::ImageChanged(WrappedImagePtr image,
                                       CanDeferInvalidation defer,
                                       const IntRect* rect) {
  SetNeedsPaintInvalidation();
  LayoutBlock::ImageChanged(image, defer, rect);
}

LayoutObject* LayoutScrollbarPart::ScrollbarStyleSource() const {
  return (!scrollbar_) ? nullptr : scrollbar_->StyleSource();
}

void LayoutScrollbarPart::SetNeedsPaintInvalidation() {
  if (scrollbar_) {
    scrollbar_->SetNeedsPaintInvalidation(kAllParts);
    return;
  }

  // This LayoutScrollbarPart is a scroll corner or a resizer.
  DCHECK_EQ(part_, kNoPart);
  if (LocalFrameView* frame_view = View()->GetFrameView()) {
    if (frame_view->IsFrameViewScrollCorner(this)) {
      frame_view->SetScrollCornerNeedsPaintInvalidation();
      return;
    }
  }

  scrollable_area_->SetScrollCornerNeedsPaintInvalidation();
}

}  // namespace blink
