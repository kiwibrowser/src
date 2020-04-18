// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_highlight_view_test_api.h"

#include "ash/wm/overview/rounded_rect_view.h"
#include "ui/views/view.h"

namespace ash {

SplitViewHighlightViewTestApi::SplitViewHighlightViewTestApi(
    SplitViewHighlightView* highlight_view)
    : highlight_view_(highlight_view) {}

SplitViewHighlightViewTestApi::~SplitViewHighlightViewTestApi() = default;

views::View* SplitViewHighlightViewTestApi::GetLeftTopView() {
  return static_cast<views::View*>(highlight_view_->left_top_);
}

views::View* SplitViewHighlightViewTestApi::GetRightBottomView() {
  return static_cast<views::View*>(highlight_view_->right_bottom_);
}

views::View* SplitViewHighlightViewTestApi::GetMiddleView() {
  return highlight_view_->middle_;
}

}  // namespace ash
