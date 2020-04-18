// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/rounded_rect_view.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_util.h"

namespace ash {

RoundedRectView::RoundedRectView(int corner_radius, SkColor background_color)
    : corner_radius_(corner_radius), background_color_(background_color) {}

RoundedRectView::~RoundedRectView() = default;

void RoundedRectView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Draw a rounded rect. Use 3 rectangles and 4 circles to create a rounded
  // rect with the least overlap between shapes. SkPath::addRoundRect is not
  // used as some artifacts show up on intel devices for
  // SplitViewDragIndicators.
  // TODO(crbug.com/824564): Once the bug is resolved we should use
  // SkPath::addRoundRect.
  const int r = corner_radius_;
  const int w = size().width();
  const int h = size().height();

  SkPath path;
  const int vertical_ext = h - r * 2;
  const int horizontal_ext = w - r * 2;

  path.addCircle(r, r, r);  // top-left circle
  if (vertical_ext > 0) {
    // bottom-left circle
    path.addCircle(r, h - r, r);
    // middle rectangle
    path.addRect(gfx::RectToSkRect(gfx::Rect(0, r, w, vertical_ext)));
  }
  if (horizontal_ext > 0) {
    // top-right circle
    path.addCircle(w - r, r, r);
    // top rectangle
    path.addRect(gfx::RectToSkRect(gfx::Rect(r, 0, horizontal_ext, r)));
    // bottom rectangle
    path.addRect(gfx::RectToSkRect(gfx::Rect(r, h - r, horizontal_ext, r)));
  }
  if (vertical_ext > 0 && horizontal_ext > 0)
    path.addCircle(w - r, h - r, r);  // bottom-right circle

  canvas->ClipPath(path, true);
  canvas->DrawColor(background_color_);
}

void RoundedRectView::SetBackgroundColor(SkColor background_color) {
  if (background_color_ == background_color)
    return;

  background_color_ = background_color;
  SchedulePaint();
}

void RoundedRectView::SetCornerRadius(int radius) {
  if (corner_radius_ == radius)
    return;

  corner_radius_ = radius;
  SchedulePaint();
}

}  // namespace ash
