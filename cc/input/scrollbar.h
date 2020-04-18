// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLLBAR_H_
#define CC_INPUT_SCROLLBAR_H_

#include "cc/cc_export.h"
#include "cc/paint/paint_canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {

enum ScrollbarOrientation { HORIZONTAL, VERTICAL };
enum ScrollDirection { SCROLL_BACKWARD, SCROLL_FORWARD };
// For now, TRACK includes everything but the thumb including background and
// buttons.
enum ScrollbarPart { THUMB, TRACK, TICKMARKS };

class Scrollbar {
 public:
  virtual ~Scrollbar() {}

  virtual ScrollbarOrientation Orientation() const = 0;
  virtual bool IsLeftSideVerticalScrollbar() const = 0;
  virtual gfx::Point Location() const = 0;
  virtual bool IsOverlay() const = 0;
  virtual bool HasThumb() const = 0;
  virtual int ThumbThickness() const = 0;
  virtual int ThumbLength() const = 0;
  virtual gfx::Rect TrackRect() const = 0;
  virtual float ThumbOpacity() const = 0;
  virtual bool HasTickmarks() const = 0;
  virtual bool NeedsPaintPart(ScrollbarPart part) const = 0;
  virtual void PaintPart(PaintCanvas* canvas,
                         ScrollbarPart part,
                         const gfx::Rect& content_rect) = 0;
  virtual bool UsesNinePatchThumbResource() const = 0;
  virtual gfx::Size NinePatchThumbCanvasSize() const = 0;
  virtual gfx::Rect NinePatchThumbAperture() const = 0;
};

}  // namespace cc

#endif  // CC_INPUT_SCROLLBAR_H_
