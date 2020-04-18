/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_mock.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar.h"

namespace blink {

static int g_c_scrollbar_thickness[] = {15, 11};

int ScrollbarThemeMock::ScrollbarThickness(ScrollbarControlSize control_size) {
  return g_c_scrollbar_thickness[control_size];
}

bool ScrollbarThemeMock::UsesOverlayScrollbars() const {
  return RuntimeEnabledFeatures::OverlayScrollbarsEnabled();
}

IntRect ScrollbarThemeMock::TrackRect(const Scrollbar& scrollbar, bool) {
  return scrollbar.FrameRect();
}

void ScrollbarThemeMock::PaintTrackBackground(GraphicsContext& context,
                                              const Scrollbar& scrollbar,
                                              const IntRect& track_rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, scrollbar, DisplayItem::kScrollbarTrackBackground))
    return;

  DrawingRecorder recorder(context, scrollbar,
                           DisplayItem::kScrollbarTrackBackground);
  context.FillRect(track_rect,
                   scrollbar.Enabled() ? Color::kLightGray : Color(0xFFE0E0E0));
}

void ScrollbarThemeMock::PaintThumb(GraphicsContext& context,
                                    const Scrollbar& scrollbar,
                                    const IntRect& thumb_rect) {
  if (!scrollbar.Enabled())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(context, scrollbar,
                                                  DisplayItem::kScrollbarThumb))
    return;

  DrawingRecorder recorder(context, scrollbar, DisplayItem::kScrollbarThumb);
  context.FillRect(thumb_rect, Color::kDarkGray);
}

void ScrollbarThemeMock::PaintScrollCorner(GraphicsContext& context,
                                           const DisplayItemClient& scrollbar,
                                           const IntRect& corner_rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, scrollbar, DisplayItem::kScrollbarCorner))
    return;

  DrawingRecorder recorder(context, scrollbar, DisplayItem::kScrollbarCorner);
  context.FillRect(corner_rect, Color::kWhite);
}

int ScrollbarThemeMock::MinimumThumbLength(const Scrollbar& scrollbar) {
  return ScrollbarThickness(scrollbar.GetControlSize());
}

}  // namespace blink
