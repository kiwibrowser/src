// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLITVIEW_SPLIT_VIEW_HIGHLIGHT_VIEW_H_
#define ASH_WM_SPLITVIEW_SPLIT_VIEW_HIGHLIGHT_VIEW_H_

#include "ash/ash_export.h"
#include "ui/views/view.h"

namespace ash {

class RoundedRectView;
class SplitViewHighlightViewTestApi;

// View that is used for displaying and animating the highlights which tell
// users where to drag windows to enter splitview, and previews the space which
// a snapped window will occupy. It is a view consisting of a rectangle with
// rounded corners on the left or top, a rectangle in the middle and a rectangle
// with rounded corners on the right or bottom. It is done this way to ensure
// rounded corners remain the same during the duration of an animation.
// (Transforming a rounded rect will stretch the corners, and having to repaint
// every animation tick is expensive.)
class ASH_EXPORT SplitViewHighlightView : public views::View {
 public:
  explicit SplitViewHighlightView(bool is_right_or_bottom);
  ~SplitViewHighlightView() override;

  void SetBounds(const gfx::Rect& bounds, bool landscape, bool animate);

  void SetColor(SkColor color);

 private:
  friend class SplitViewHighlightViewTestApi;

  // The three components of this view.
  RoundedRectView* left_top_ = nullptr;
  RoundedRectView* right_bottom_ = nullptr;
  views::View* middle_ = nullptr;

  bool landscape_ = true;
  // Determines whether this particular highlight view is located at the right
  // or bottom of the screen. These highlights animate in the opposite direction
  // as left or top highlights, so when we use SetBounds extra calucations have
  // to be done to ensure the animation is correct.
  const bool is_right_or_bottom_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewHighlightView);
};

}  // namespace ash

#endif  // ASH_WM_SPLITVIEW_SPLIT_VIEW_HIGHLIGHT_VIEW_H_