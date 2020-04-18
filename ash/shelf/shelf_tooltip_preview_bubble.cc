// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_preview_bubble.h"

namespace ash {

constexpr int kTooltipMaxDimension = 250;
constexpr int kTooltipMargin = 1;

ShelfTooltipPreviewBubble::ShelfTooltipPreviewBubble(
    views::View* anchor,
    views::BubbleBorder::Arrow arrow,
    aura::Window* window)
    : views::BubbleDialogDelegateView(anchor, arrow) {
  preview_ =
      new wm::WindowMirrorView(window, /* trilinear_filtering_on_init */ false);

  gfx::Size preview_size = preview_->CalculatePreferredSize();
  float preview_ratio = static_cast<float>(preview_size.width()) /
                        static_cast<float>(preview_size.height());
  int preview_height = kTooltipMaxDimension;
  int preview_width = kTooltipMaxDimension;
  if (preview_ratio > 1) {
    preview_height = kTooltipMaxDimension / preview_ratio;
  } else {
    preview_width = kTooltipMaxDimension * preview_ratio;
  }
  preview_->SetBoundsRect(gfx::Rect(gfx::Size(preview_width, preview_height)));

  AddChildView(preview_);

  // Place the bubble in the same display as the anchor.
  set_parent_window(
      anchor_widget()->GetNativeWindow()->GetRootWindow()->GetChildById(
          kShellWindowId_SettingBubbleContainer));
  set_margins(gfx::Insets(kTooltipMargin, kTooltipMargin));

  views::BubbleDialogDelegateView::CreateBubble(this);
}

gfx::Size ShelfTooltipPreviewBubble::CalculatePreferredSize() const {
  if (preview_ == nullptr) {
    return BubbleDialogDelegateView::CalculatePreferredSize();
  }
  // TODO: This is mostly a placeholder for a very first version. Compute this
  // properly.
  return gfx::Size(kTooltipMaxDimension, kTooltipMaxDimension);
}

int ShelfTooltipPreviewBubble::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

}  // namespace ash
