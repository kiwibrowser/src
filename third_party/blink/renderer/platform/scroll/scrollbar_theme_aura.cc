/*
 * Copyright (c) 2008, 2009, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT{
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,{
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_aura.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_theme_engine.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_overlay.h"

namespace blink {

namespace {

static bool UseMockTheme() {
  return LayoutTestSupport::IsRunningLayoutTest();
}

// Contains a flag indicating whether WebThemeEngine should paint a UI widget
// for a scrollbar part, and if so, what part and state apply.
//
// If the PartPaintingParams are not affected by a change in the scrollbar
// state, then the corresponding scrollbar part does not need to be repainted.
struct PartPaintingParams {
  PartPaintingParams()
      : should_paint(false),
        part(WebThemeEngine::kPartScrollbarDownArrow),
        state(WebThemeEngine::kStateNormal) {}
  PartPaintingParams(WebThemeEngine::Part part, WebThemeEngine::State state)
      : should_paint(true), part(part), state(state) {}

  bool should_paint;
  WebThemeEngine::Part part;
  WebThemeEngine::State state;
};

bool operator==(const PartPaintingParams& a, const PartPaintingParams& b) {
  return (!a.should_paint && !b.should_paint) ||
         std::tie(a.should_paint, a.part, a.state) ==
             std::tie(b.should_paint, b.part, b.state);
}

bool operator!=(const PartPaintingParams& a, const PartPaintingParams& b) {
  return !(a == b);
}

PartPaintingParams ButtonPartPaintingParams(const Scrollbar& scrollbar,
                                            float position,
                                            ScrollbarPart part) {
  WebThemeEngine::Part paint_part;
  WebThemeEngine::State state = WebThemeEngine::kStateNormal;
  bool check_min = false;
  bool check_max = false;

  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    if (part == kBackButtonStartPart) {
      paint_part = WebThemeEngine::kPartScrollbarLeftArrow;
      check_min = true;
    } else if (UseMockTheme() && part != kForwardButtonEndPart) {
      return PartPaintingParams();
    } else {
      paint_part = WebThemeEngine::kPartScrollbarRightArrow;
      check_max = true;
    }
  } else {
    if (part == kBackButtonStartPart) {
      paint_part = WebThemeEngine::kPartScrollbarUpArrow;
      check_min = true;
    } else if (UseMockTheme() && part != kForwardButtonEndPart) {
      return PartPaintingParams();
    } else {
      paint_part = WebThemeEngine::kPartScrollbarDownArrow;
      check_max = true;
    }
  }

  if (UseMockTheme() && !scrollbar.Enabled()) {
    state = WebThemeEngine::kStateDisabled;
  } else if (!UseMockTheme() &&
             ((check_min && (position <= 0)) ||
              (check_max && position >= scrollbar.Maximum()))) {
    state = WebThemeEngine::kStateDisabled;
  } else {
    if (part == scrollbar.PressedPart())
      state = WebThemeEngine::kStatePressed;
    else if (part == scrollbar.HoveredPart())
      state = WebThemeEngine::kStateHover;
  }

  return PartPaintingParams(paint_part, state);
}

static int GetScrollbarThickness() {
  return Platform::Current()
      ->ThemeEngine()
      ->GetSize(WebThemeEngine::kPartScrollbarVerticalThumb)
      .width;
}

}  // namespace

ScrollbarTheme& ScrollbarTheme::NativeTheme() {
  if (RuntimeEnabledFeatures::OverlayScrollbarsEnabled()) {
    DEFINE_STATIC_LOCAL(
        ScrollbarThemeOverlay, theme,
        (GetScrollbarThickness(), 0, ScrollbarThemeOverlay::kAllowHitTest));
    return theme;
  }

  DEFINE_STATIC_LOCAL(ScrollbarThemeAura, theme, ());
  return theme;
}

int ScrollbarThemeAura::ScrollbarThickness(ScrollbarControlSize control_size) {
  // Horiz and Vert scrollbars are the same thickness.
  // In unit tests we don't have the mock theme engine (because of layering
  // violations), so we hard code the size (see bug 327470).
  if (UseMockTheme())
    return 15;
  IntSize scrollbar_size = Platform::Current()->ThemeEngine()->GetSize(
      WebThemeEngine::kPartScrollbarVerticalTrack);
  return scrollbar_size.Width();
}

bool ScrollbarThemeAura::HasThumb(const Scrollbar& scrollbar) {
  // This method is just called as a paint-time optimization to see if
  // painting the thumb can be skipped. We don't have to be exact here.
  return ThumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeAura::BackButtonRect(const Scrollbar& scrollbar,
                                           ScrollbarPart part,
                                           bool) {
  // Windows and Linux just have single arrows.
  if (part == kBackButtonEndPart)
    return IntRect();

  IntSize size = ButtonSize(scrollbar);
  return IntRect(scrollbar.X(), scrollbar.Y(), size.Width(), size.Height());
}

IntRect ScrollbarThemeAura::ForwardButtonRect(const Scrollbar& scrollbar,
                                              ScrollbarPart part,
                                              bool) {
  // Windows and Linux just have single arrows.
  if (part == kForwardButtonStartPart)
    return IntRect();

  IntSize size = ButtonSize(scrollbar);
  int x, y;
  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    x = scrollbar.X() + scrollbar.Width() - size.Width();
    y = scrollbar.Y();
  } else {
    x = scrollbar.X();
    y = scrollbar.Y() + scrollbar.Height() - size.Height();
  }
  return IntRect(x, y, size.Width(), size.Height());
}

IntRect ScrollbarThemeAura::TrackRect(const Scrollbar& scrollbar, bool) {
  // The track occupies all space between the two buttons.
  IntSize bs = ButtonSize(scrollbar);
  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    if (scrollbar.Width() <= 2 * bs.Width())
      return IntRect();
    return IntRect(scrollbar.X() + bs.Width(), scrollbar.Y(),
                   scrollbar.Width() - 2 * bs.Width(), scrollbar.Height());
  }
  if (scrollbar.Height() <= 2 * bs.Height())
    return IntRect();
  return IntRect(scrollbar.X(), scrollbar.Y() + bs.Height(), scrollbar.Width(),
                 scrollbar.Height() - 2 * bs.Height());
}

int ScrollbarThemeAura::MinimumThumbLength(const Scrollbar& scrollbar) {
  if (scrollbar.Orientation() == kVerticalScrollbar) {
    return Platform::Current()
        ->ThemeEngine()
        ->GetSize(WebThemeEngine::kPartScrollbarVerticalThumb)
        .height;
  }

  return Platform::Current()
      ->ThemeEngine()
      ->GetSize(WebThemeEngine::kPartScrollbarHorizontalThumb)
      .width;
}

void ScrollbarThemeAura::PaintTrackBackground(GraphicsContext& context,
                                              const Scrollbar& scrollbar,
                                              const IntRect& rect) {
  // Just assume a forward track part. We only paint the track as a single piece
  // when there is no thumb.
  if (!HasThumb(scrollbar) && !rect.IsEmpty())
    PaintTrackPiece(context, scrollbar, rect, kForwardTrackPart);
}

void ScrollbarThemeAura::PaintTrackPiece(GraphicsContext& gc,
                                         const Scrollbar& scrollbar,
                                         const IntRect& rect,
                                         ScrollbarPart part_type) {
  DisplayItem::Type display_item_type =
      TrackPiecePartToDisplayItemType(part_type);
  if (DrawingRecorder::UseCachedDrawingIfPossible(gc, scrollbar,
                                                  display_item_type))
    return;

  DrawingRecorder recorder(gc, scrollbar, display_item_type);

  WebThemeEngine::State state = scrollbar.HoveredPart() == part_type
                                    ? WebThemeEngine::kStateHover
                                    : WebThemeEngine::kStateNormal;

  if (UseMockTheme() && !scrollbar.Enabled())
    state = WebThemeEngine::kStateDisabled;

  IntRect align_rect = TrackRect(scrollbar, false);
  WebThemeEngine::ExtraParams extra_params;
  extra_params.scrollbar_track.is_back = (part_type == kBackTrackPart);
  extra_params.scrollbar_track.track_x = align_rect.X();
  extra_params.scrollbar_track.track_y = align_rect.Y();
  extra_params.scrollbar_track.track_width = align_rect.Width();
  extra_params.scrollbar_track.track_height = align_rect.Height();
  Platform::Current()->ThemeEngine()->Paint(
      gc.Canvas(),
      scrollbar.Orientation() == kHorizontalScrollbar
          ? WebThemeEngine::kPartScrollbarHorizontalTrack
          : WebThemeEngine::kPartScrollbarVerticalTrack,
      state, WebRect(rect), &extra_params);
}

void ScrollbarThemeAura::PaintButton(GraphicsContext& gc,
                                     const Scrollbar& scrollbar,
                                     const IntRect& rect,
                                     ScrollbarPart part) {
  DisplayItem::Type display_item_type = ButtonPartToDisplayItemType(part);
  if (DrawingRecorder::UseCachedDrawingIfPossible(gc, scrollbar,
                                                  display_item_type))
    return;
  PartPaintingParams params =
      ButtonPartPaintingParams(scrollbar, scrollbar.CurrentPos(), part);
  if (!params.should_paint)
    return;
  DrawingRecorder recorder(gc, scrollbar, display_item_type);
  Platform::Current()->ThemeEngine()->Paint(
      gc.Canvas(), params.part, params.state, WebRect(rect), nullptr);
}

void ScrollbarThemeAura::PaintThumb(GraphicsContext& gc,
                                    const Scrollbar& scrollbar,
                                    const IntRect& rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(gc, scrollbar,
                                                  DisplayItem::kScrollbarThumb))
    return;

  DrawingRecorder recorder(gc, scrollbar, DisplayItem::kScrollbarThumb);

  WebThemeEngine::State state;
  WebCanvas* canvas = gc.Canvas();
  if (scrollbar.PressedPart() == kThumbPart)
    state = WebThemeEngine::kStatePressed;
  else if (scrollbar.HoveredPart() == kThumbPart)
    state = WebThemeEngine::kStateHover;
  else
    state = WebThemeEngine::kStateNormal;

  Platform::Current()->ThemeEngine()->Paint(
      canvas,
      scrollbar.Orientation() == kHorizontalScrollbar
          ? WebThemeEngine::kPartScrollbarHorizontalThumb
          : WebThemeEngine::kPartScrollbarVerticalThumb,
      state, WebRect(rect), nullptr);
}

bool ScrollbarThemeAura::ShouldRepaintAllPartsOnInvalidation() const {
  // This theme can separately handle thumb invalidation.
  return false;
}

ScrollbarPart ScrollbarThemeAura::InvalidateOnThumbPositionChange(
    const Scrollbar& scrollbar,
    float old_position,
    float new_position) const {
  ScrollbarPart invalid_parts = kNoPart;
  DCHECK_EQ(ButtonsPlacement(), kWebScrollbarButtonsPlacementSingle);
  static const ScrollbarPart kButtonParts[] = {kBackButtonStartPart,
                                               kForwardButtonEndPart};
  for (ScrollbarPart part : kButtonParts) {
    if (ButtonPartPaintingParams(scrollbar, old_position, part) !=
        ButtonPartPaintingParams(scrollbar, new_position, part))
      invalid_parts = static_cast<ScrollbarPart>(invalid_parts | part);
  }
  return invalid_parts;
}

bool ScrollbarThemeAura::HasScrollbarButtons(
    ScrollbarOrientation orientation) const {
  WebThemeEngine* theme_engine = Platform::Current()->ThemeEngine();
  if (orientation == kVerticalScrollbar) {
    return !theme_engine->GetSize(WebThemeEngine::kPartScrollbarDownArrow)
                .IsEmpty();
  }
  return !theme_engine->GetSize(WebThemeEngine::kPartScrollbarLeftArrow)
              .IsEmpty();
};

IntSize ScrollbarThemeAura::ButtonSize(const Scrollbar& scrollbar) {
  if (!HasScrollbarButtons(scrollbar.Orientation()))
    return IntSize(0, 0);

  if (scrollbar.Orientation() == kVerticalScrollbar) {
    int square_size = scrollbar.Width();
    return IntSize(square_size, scrollbar.Height() < 2 * square_size
                                    ? scrollbar.Height() / 2
                                    : square_size);
  }

  // HorizontalScrollbar
  int square_size = scrollbar.Height();
  return IntSize(
      scrollbar.Width() < 2 * square_size ? scrollbar.Width() / 2 : square_size,
      square_size);
}

}  // namespace blink
