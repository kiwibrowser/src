// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_ROUNDED_RECT_VIEW_H_
#define ASH_WM_OVERVIEW_ROUNDED_RECT_VIEW_H_

#include "ui/views/view.h"

namespace gfx {
class Canvas;
}  // namespace gfx

namespace ash {

// A View having rounded corners and a specified background color which is
// only painted within the bounds defined by the rounded corners.
// TODO(varkha): This duplicates code from RoundedImageView. Refactor these
//               classes and move into ui/views.
class RoundedRectView : public views::View {
 public:
  RoundedRectView(int corner_radius, SkColor background_color);
  ~RoundedRectView() override;

  void SetBackgroundColor(SkColor background_color);
  void SetCornerRadius(int radius);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  int corner_radius_;
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(RoundedRectView);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_ROUNDED_RECT_VIEW_H_
