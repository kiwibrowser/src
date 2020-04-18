// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"

#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

BackgroundWith1PxBorder::BackgroundWith1PxBorder(SkColor background,
                                                 SkColor border)
    : border_color_(border) {
  SetNativeControlColor(background);
}

void BackgroundWith1PxBorder::PaintFocusRing(gfx::Canvas* canvas,
                                             ui::NativeTheme* theme,
                                             const gfx::Rect& local_bounds) {
  SkColor focus_ring_color = theme->GetSystemColor(
      ui::NativeTheme::NativeTheme::kColorId_FocusedBorderColor);
  Paint(canvas, SK_ColorTRANSPARENT, focus_ring_color,
        GetBorderRadius(local_bounds.height() * canvas->image_scale()),
        local_bounds);
}

void BackgroundWith1PxBorder::Paint(gfx::Canvas* canvas,
                                    views::View* view) const {
  Paint(canvas, get_color(), border_color_,
        GetBorderRadius(view->height() * canvas->image_scale()),
        view->GetContentsBounds());
}

float BackgroundWith1PxBorder::GetBorderRadius(int height_in_px) const {
  if (LocationBarView::IsRounded()) {
    // This method returns the inner radius of the border, so subtract 1 pixel
    // off the final border radius since the border thickness is always 1px.
    return height_in_px / 2.f - 1;
  }
  return kLegacyBorderRadiusPx;
}

void BackgroundWith1PxBorder::Paint(gfx::Canvas* canvas,
                                    SkColor background,
                                    SkColor border,
                                    float inner_border_radius,
                                    const gfx::Rect& bounds) const {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();
  gfx::RectF border_rect_f(bounds);
  border_rect_f.Scale(scale);

  // Inset by |kBorderThicknessDip|, then draw the border along the outside edge
  // of the result. Make sure the inset amount is a whole number so the border
  // will still be aligned to the pixel grid. std::floor is chosen here to
  // ensure the border will be fully contained within the |kBorderThicknessDip|
  // region.
  border_rect_f.Inset(gfx::InsetsF(std::floor(kBorderThicknessDip * scale)));

  SkRRect inner_rect(SkRRect::MakeRectXY(gfx::RectFToSkRect(border_rect_f),
                                         inner_border_radius,
                                         inner_border_radius));
  SkRRect outer_rect(inner_rect);
  // The border is 1px thick regardless of scale factor, so hard code that here.
  outer_rect.outset(1, 1);

  cc::PaintFlags flags;
  flags.setBlendMode(blend_mode_);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  flags.setColor(border);
  canvas->sk_canvas()->drawDRRect(outer_rect, inner_rect, flags);

  flags.setColor(background);
  canvas->sk_canvas()->drawRRect(inner_rect, flags);
}
