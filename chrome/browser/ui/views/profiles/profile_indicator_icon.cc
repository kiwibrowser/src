// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"

#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "ui/gfx/canvas.h"

ProfileIndicatorIcon::ProfileIndicatorIcon() {
  // In RTL mode, the incognito icon should be looking the opposite direction.
  EnableCanvasFlippingForRTLUI(true);
}

ProfileIndicatorIcon::~ProfileIndicatorIcon() {}

void ProfileIndicatorIcon::OnPaint(gfx::Canvas* canvas) {
  if (base_icon_.IsEmpty())
    return;

  if (old_height_ != height() || modified_icon_.isNull()) {
    old_height_ = height();
    modified_icon_ = *profiles::GetAvatarIconForTitleBar(base_icon_, false,
                                                         width(), height())
                          .ToImageSkia();
  }

  // Scale the image to fit the width of the button.
  int dst_width = std::min(modified_icon_.width(), width());
  // Truncate rather than rounding, so that for odd widths we put the extra
  // pixel on the left.
  int dst_x = (width() - dst_width) / 2;

  // Scale the height and maintain aspect ratio. This means that the
  // icon may not fit in the view. That's ok, we just vertically center it.
  float scale = static_cast<float>(dst_width) /
                static_cast<float>(modified_icon_.width());
  // Round here so that we minimize the aspect ratio drift.
  int dst_height = std::round(modified_icon_.height() * scale);
  // Round rather than truncating, so that for odd heights we select an extra
  // pixel below the image center rather than above.  This is because the
  // incognito image has shadows at the top that make the apparent center below
  // the real center.
  int dst_y = std::round((height() - dst_height) / 2.0f);
  canvas->DrawImageInt(modified_icon_, 0, 0, modified_icon_.width(),
                       modified_icon_.height(), dst_x, dst_y, dst_width,
                       dst_height, false);

  if (stroke_color_ == SK_ColorTRANSPARENT)
    return;

  // Draw a 1px circular stroke (regardless of DSF) around the avatar icon.
  const gfx::PointF center((dst_x + dst_width) / 2.0f,
                           (dst_y + dst_height) / 2.0f);
  cc::PaintFlags paint_flags;
  paint_flags.setAntiAlias(true);
  paint_flags.setStyle(cc::PaintFlags::kStroke_Style);
  paint_flags.setColor(stroke_color_);
  paint_flags.setStrokeWidth(1.0f / canvas->image_scale());
  // Reduce the radius by 0.5f such that the circle overlaps with the edge of
  // the image.
  const float radius = (dst_width + dst_height) / 4.0f;
  canvas->DrawCircle(center, radius - 0.5f, paint_flags);
}

void ProfileIndicatorIcon::SetIcon(const gfx::Image& icon) {
  base_icon_ = icon;
  modified_icon_ = gfx::ImageSkia();
  SchedulePaint();
}
