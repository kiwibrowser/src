// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WINDOW_FRAME_BACKGROUND_H_
#define UI_VIEWS_WINDOW_FRAME_BACKGROUND_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/views_export.h"

namespace gfx {
class Canvas;
}

namespace views {

class View;

// FrameBackground handles painting for all the various window frames we
// support in Chrome. It intends to consolidate paint code that historically
// was copied. One frame to rule them all!
class VIEWS_EXPORT FrameBackground {
 public:
  FrameBackground();
  ~FrameBackground();

  // Sets the color to draw under the frame images.
  void set_frame_color(SkColor color) { frame_color_ = color; }

  void set_use_custom_frame(bool use_custom_frame) {
    use_custom_frame_ = use_custom_frame;
  }

  // Sets whether the frame to be drawn should have focus.
  void set_is_active(bool is_active) { is_active_ = is_active; }

  // Sets whether the frame to be drawn is in incognito mode.
  void set_incognito(bool incognito) { incognito_ = incognito; }

  // Sets the theme image for the top of the window.  May be null (empty).
  // Memory is owned by the caller.
  void set_theme_image(const gfx::ImageSkia& image) { theme_image_ = image; }

  // Sets an image that overlays the top window image.  Usually used to add
  // edge highlighting to provide the illusion of depth.  May be null (empty).
  // Memory is owned by the caller.
  void set_theme_overlay_image(const gfx::ImageSkia& image) {
    theme_overlay_image_ = image;
  }

  // Sets the height of the top area to fill with the default frame color,
  // which must extend behind the tab strip.
  void set_top_area_height(int height) { top_area_height_ = height; }

  // Vertical inset for theme image when drawing maximized.
  void set_maximized_top_inset(int inset) { maximized_top_inset_ = inset; }

  // Sets images used when drawing the sides of the frame.
  // Caller owns the memory.
  void SetSideImages(const gfx::ImageSkia* left,
                     const gfx::ImageSkia* top,
                     const gfx::ImageSkia* right,
                     const gfx::ImageSkia* bottom);

  // Sets images used when drawing the corners of the frame.
  // Caller owns the memory.
  void SetCornerImages(const gfx::ImageSkia* top_left,
                       const gfx::ImageSkia* top_right,
                       const gfx::ImageSkia* bottom_left,
                       const gfx::ImageSkia* bottom_right);

  // Paints the border for a standard, non-maximized window.  Also paints the
  // background of the title bar area, since the top frame border and the
  // title bar background are a contiguous component.
  void PaintRestored(gfx::Canvas* canvas, const View* view) const;

  // Paints the border for a maximized window, which does not include the
  // window edges.
  void PaintMaximized(gfx::Canvas* canvas, const View* view) const;

 private:
  // Fills the frame area with the frame color.
  void PaintFrameColor(gfx::Canvas* canvas, const View* view) const;

  // Paints the background of the tab strip.
  void PaintFrameTopArea(gfx::Canvas* canvas, const View* view) const;

  SkColor frame_color_;
  bool use_custom_frame_;
  bool is_active_;
  bool incognito_;
  gfx::ImageSkia theme_image_;
  gfx::ImageSkia theme_overlay_image_;
  int top_area_height_;

  // Images for the sides of the frame.
  const gfx::ImageSkia* left_edge_;
  const gfx::ImageSkia* top_edge_;
  const gfx::ImageSkia* right_edge_;
  const gfx::ImageSkia* bottom_edge_;

  // Images for the corners of the frame.
  const gfx::ImageSkia* top_left_corner_;
  const gfx::ImageSkia* top_right_corner_;
  const gfx::ImageSkia* bottom_left_corner_;
  const gfx::ImageSkia* bottom_right_corner_;

  // Vertical inset for theme image when drawing maximized.
  int maximized_top_inset_;

  DISALLOW_COPY_AND_ASSIGN(FrameBackground);
};

}  // namespace views

#endif  // UI_VIEWS_WINDOW_FRAME_BACKGROUND_H_
