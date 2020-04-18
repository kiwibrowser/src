// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_BACKGROUND_WITH_1_PX_BORDER_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_BACKGROUND_WITH_1_PX_BORDER_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/background.h"

namespace gfx {
class Canvas;
}

namespace views {
class View;
}

// BackgroundWith1PxBorder renders a solid background color, with a one pixel
// border with rounded corners. This accounts for the scaling of the canvas, so
// that the border is one pixel regardless of display scaling.
// TODO(patricialor): Delete this & replace with CreateRoundRectWith1PxPainter.
class BackgroundWith1PxBorder : public views::Background {
 public:
  // The thickness of the border in DIP.
  static constexpr int kBorderThicknessDip = 1;

  // The legacy (non touch/material) border radius.
  static constexpr int kLegacyBorderRadiusPx = 2;

  BackgroundWith1PxBorder(SkColor background, SkColor border);

  void set_blend_mode(SkBlendMode blend_mode) { blend_mode_ = blend_mode; }

  // Paints a blue focus ring that draws over the top of the existing border.
  void PaintFocusRing(gfx::Canvas* canvas,
                      ui::NativeTheme* theme,
                      const gfx::Rect& local_bounds);

  // views::Background:
  void Paint(gfx::Canvas* canvas, views::View* view) const override;

 protected:
  // Returns the amount of border radius to use for the inside edge of the
  // border stroke for this background.
  float GetBorderRadius(int height_in_px) const;

  // Paints the background. |inner_border_radius| is the border radius of the
  // inside of the stroke.
  void Paint(gfx::Canvas* canvas,
             SkColor background,
             SkColor border,
             float inner_border_radius,
             const gfx::Rect& bounds) const;

 private:
  // Color for the one pixel border.
  SkColor border_color_;

  // Blend mode used when painting.
  SkBlendMode blend_mode_ = SkBlendMode::kSrcOver;

  DISALLOW_COPY_AND_ASSIGN(BackgroundWith1PxBorder);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_BACKGROUND_WITH_1_PX_BORDER_H_
