// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_scrollbar.h"

#include "cc/paint/paint_flags.h"
#include "ui/gfx/skia_util.h"

namespace cc {

FakeScrollbar::FakeScrollbar()
    : FakeScrollbar(false, false, HORIZONTAL, false, false) {}

FakeScrollbar::FakeScrollbar(bool paint, bool has_thumb, bool is_overlay)
    : FakeScrollbar(paint, has_thumb, HORIZONTAL, false, is_overlay) {}

FakeScrollbar::FakeScrollbar(bool paint,
                             bool has_thumb,
                             ScrollbarOrientation orientation,
                             bool is_left_side_vertical_scrollbar,
                             bool is_overlay)
    : paint_(paint),
      has_thumb_(has_thumb),
      orientation_(orientation),
      is_left_side_vertical_scrollbar_(is_left_side_vertical_scrollbar),
      is_overlay_(is_overlay),
      thumb_thickness_(10),
      thumb_length_(5),
      thumb_opacity_(1),
      needs_paint_thumb_(true),
      needs_paint_track_(true),
      has_tickmarks_(false),
      track_rect_(0, 0, 100, 10),
      fill_color_(SK_ColorGREEN) {}

FakeScrollbar::~FakeScrollbar() = default;

ScrollbarOrientation FakeScrollbar::Orientation() const {
  return orientation_;
}

bool FakeScrollbar::IsLeftSideVerticalScrollbar() const {
  return is_left_side_vertical_scrollbar_;
}

gfx::Point FakeScrollbar::Location() const { return location_; }

bool FakeScrollbar::IsOverlay() const { return is_overlay_; }

bool FakeScrollbar::HasThumb() const { return has_thumb_; }

int FakeScrollbar::ThumbThickness() const {
  return thumb_thickness_;
}

int FakeScrollbar::ThumbLength() const {
  return thumb_length_;
}

gfx::Rect FakeScrollbar::TrackRect() const {
  return track_rect_;
}

float FakeScrollbar::ThumbOpacity() const {
  return thumb_opacity_;
}

bool FakeScrollbar::NeedsPaintPart(ScrollbarPart part) const {
  if (part == THUMB)
    return needs_paint_thumb_;
  return needs_paint_track_;
}

bool FakeScrollbar::HasTickmarks() const {
  return has_tickmarks_;
}

void FakeScrollbar::PaintPart(PaintCanvas* canvas,
                              ScrollbarPart part,
                              const gfx::Rect& content_rect) {
  if (!paint_)
    return;

  // Fill the scrollbar with a different color each time.
  fill_color_++;
  PaintFlags flags;
  flags.setAntiAlias(false);
  flags.setColor(paint_fill_color());
  flags.setStyle(PaintFlags::kFill_Style);

  // Emulate the how the real scrollbar works by using scrollbar's rect for
  // TRACK and the given content_rect for the THUMB
  SkRect rect = part == TRACK ? RectToSkRect(TrackRect())
                              : RectToSkRect(content_rect);
  canvas->drawRect(rect, flags);
}

bool FakeScrollbar::UsesNinePatchThumbResource() const {
  return false;
}

gfx::Size FakeScrollbar::NinePatchThumbCanvasSize() const {
  return gfx::Size();
}

gfx::Rect FakeScrollbar::NinePatchThumbAperture() const {
  return gfx::Rect();
}

}  // namespace cc
