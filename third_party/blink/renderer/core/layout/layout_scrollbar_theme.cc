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

#include "third_party/blink/renderer/core/layout/layout_scrollbar_theme.h"

#include "third_party/blink/renderer/core/layout/layout_scrollbar.h"
#include "third_party/blink/renderer/core/paint/scrollbar_painter.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"

namespace blink {

LayoutScrollbarTheme* LayoutScrollbarTheme::GetLayoutScrollbarTheme() {
  DEFINE_STATIC_LOCAL(LayoutScrollbarTheme, theme, ());
  return &theme;
}

void LayoutScrollbarTheme::ButtonSizesAlongTrackAxis(const Scrollbar& scrollbar,
                                                     int& before_size,
                                                     int& after_size) {
  IntRect first_button = BackButtonRect(scrollbar, kBackButtonStartPart);
  IntRect second_button = ForwardButtonRect(scrollbar, kForwardButtonStartPart);
  IntRect third_button = BackButtonRect(scrollbar, kBackButtonEndPart);
  IntRect fourth_button = ForwardButtonRect(scrollbar, kForwardButtonEndPart);
  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    before_size = first_button.Width() + second_button.Width();
    after_size = third_button.Width() + fourth_button.Width();
  } else {
    before_size = first_button.Height() + second_button.Height();
    after_size = third_button.Height() + fourth_button.Height();
  }
}

bool LayoutScrollbarTheme::HasButtons(const Scrollbar& scrollbar) {
  int start_size;
  int end_size;
  ButtonSizesAlongTrackAxis(scrollbar, start_size, end_size);
  return (start_size + end_size) <=
         (scrollbar.Orientation() == kHorizontalScrollbar ? scrollbar.Width()
                                                          : scrollbar.Height());
}

bool LayoutScrollbarTheme::HasThumb(const Scrollbar& scrollbar) {
  return TrackLength(scrollbar) - ThumbLength(scrollbar) >= 0;
}

int LayoutScrollbarTheme::MinimumThumbLength(const Scrollbar& scrollbar) {
  return ToLayoutScrollbar(scrollbar).MinimumThumbLength();
}

IntRect LayoutScrollbarTheme::BackButtonRect(const Scrollbar& scrollbar,
                                             ScrollbarPart part_type,
                                             bool) {
  return ToLayoutScrollbar(scrollbar).ButtonRect(part_type);
}

IntRect LayoutScrollbarTheme::ForwardButtonRect(const Scrollbar& scrollbar,
                                                ScrollbarPart part_type,
                                                bool) {
  return ToLayoutScrollbar(scrollbar).ButtonRect(part_type);
}

IntRect LayoutScrollbarTheme::TrackRect(const Scrollbar& scrollbar, bool) {
  if (!HasButtons(scrollbar))
    return scrollbar.FrameRect();

  int start_length;
  int end_length;
  ButtonSizesAlongTrackAxis(scrollbar, start_length, end_length);

  return ToLayoutScrollbar(scrollbar).TrackRect(start_length, end_length);
}

IntRect LayoutScrollbarTheme::ConstrainTrackRectToTrackPieces(
    const Scrollbar& scrollbar,
    const IntRect& rect) {
  IntRect back_rect = ToLayoutScrollbar(scrollbar).TrackPieceRectWithMargins(
      kBackTrackPart, rect);
  IntRect forward_rect = ToLayoutScrollbar(scrollbar).TrackPieceRectWithMargins(
      kForwardTrackPart, rect);
  IntRect result = rect;
  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    result.SetX(back_rect.X());
    result.SetWidth(forward_rect.MaxX() - back_rect.X());
  } else {
    result.SetY(back_rect.Y());
    result.SetHeight(forward_rect.MaxY() - back_rect.Y());
  }
  return result;
}

void LayoutScrollbarTheme::PaintScrollCorner(
    GraphicsContext& context,
    const DisplayItemClient& display_item_client,
    const IntRect& corner_rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, display_item_client, DisplayItem::kScrollbarCorner))
    return;

  DrawingRecorder recorder(context, display_item_client,
                           DisplayItem::kScrollbarCorner);
  // FIXME: Implement.
  context.FillRect(corner_rect, Color::kWhite);
}

void LayoutScrollbarTheme::PaintScrollbarBackground(
    GraphicsContext& context,
    const Scrollbar& scrollbar) {
  ScrollbarPainter(ToLayoutScrollbar(scrollbar))
      .PaintPart(context, kScrollbarBGPart, scrollbar.FrameRect());
}

void LayoutScrollbarTheme::PaintTrackBackground(GraphicsContext& context,
                                                const Scrollbar& scrollbar,
                                                const IntRect& rect) {
  ScrollbarPainter(ToLayoutScrollbar(scrollbar))
      .PaintPart(context, kTrackBGPart, rect);
}

void LayoutScrollbarTheme::PaintTrackPiece(GraphicsContext& context,
                                           const Scrollbar& scrollbar,
                                           const IntRect& rect,
                                           ScrollbarPart part) {
  ScrollbarPainter(ToLayoutScrollbar(scrollbar)).PaintPart(context, part, rect);
}

void LayoutScrollbarTheme::PaintButton(GraphicsContext& context,
                                       const Scrollbar& scrollbar,
                                       const IntRect& rect,
                                       ScrollbarPart part) {
  ScrollbarPainter(ToLayoutScrollbar(scrollbar)).PaintPart(context, part, rect);
}

void LayoutScrollbarTheme::PaintThumb(GraphicsContext& context,
                                      const Scrollbar& scrollbar,
                                      const IntRect& rect) {
  ScrollbarPainter(ToLayoutScrollbar(scrollbar))
      .PaintPart(context, kThumbPart, rect);
}

void LayoutScrollbarTheme::PaintTickmarks(GraphicsContext& context,
                                          const Scrollbar& scrollbar,
                                          const IntRect& rect) {
  ScrollbarTheme::DeprecatedStaticGetTheme().PaintTickmarks(context, scrollbar,
                                                            rect);
}

}  // namespace blink
