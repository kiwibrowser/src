// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_base_bubble_view.h"

#include "ui/views/layout/box_layout.h"

namespace ash {
namespace {

// Total width of the bubble view.
constexpr int kBubbleTotalWidthDp = 178;

// Horizontal margin of the bubble view.
constexpr int kBubbleHorizontalMarginDp = 14;

// Top margin of the bubble view.
constexpr int kBubbleTopMarginDp = 13;

// Bottom margin of the bubble view.
constexpr int kBubbleBottomMarginDp = 18;

}  // namespace

LoginBaseBubbleView::LoginBaseBubbleView(views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::NONE) {
  set_margins(gfx::Insets(kBubbleTopMarginDp, kBubbleHorizontalMarginDp,
                          kBubbleBottomMarginDp, kBubbleHorizontalMarginDp));
  set_color(SK_ColorBLACK);
  set_can_activate(false);
  set_close_on_deactivate(false);

  // Layer rendering is needed for animation.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

LoginBaseBubbleView::~LoginBaseBubbleView() = default;

int LoginBaseBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

gfx::Size LoginBaseBubbleView::CalculatePreferredSize() const {
  gfx::Size size = views::BubbleDialogDelegateView::CalculatePreferredSize();
  size.set_width(kBubbleTotalWidthDp);
  return size;
}

}  // namespace ash
