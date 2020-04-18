// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_PREVIEW_BUBBLE_H_
#define ASH_SHELF_SHELF_TOOLTIP_PREVIEW_BUBBLE_H_

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/window_mirror_view.h"
#include "ui/aura/window.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace ash {

// The implementation of tooltip bubbles for the shelf item.
class ASH_EXPORT ShelfTooltipPreviewBubble
    : public views::BubbleDialogDelegateView {
 public:
  ShelfTooltipPreviewBubble(views::View* anchor,
                            views::BubbleBorder::Arrow arrow,
                            aura::Window* window);

 private:
  // BubbleDialogDelegateView overrides:
  gfx::Size CalculatePreferredSize() const override;
  int GetDialogButtons() const override;

  // The window preview that this tooltip is meant to display.
  wm::WindowMirrorView* preview_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipPreviewBubble);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_PREVIEW_BUBBLE_H_
