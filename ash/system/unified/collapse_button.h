// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_COLLAPSE_BUTTON_H_
#define ASH_SYSTEM_UNIFIED_COLLAPSE_BUTTON_H_

#include "ui/views/controls/button/image_button.h"

namespace ash {

// Collapse button shown in TopShortcutsView with TopShortcutButtons.
// UnifiedSystemTrayBubble will support collapsed state where the height of the
// bubble is smaller, and some rows and labels will be omitted.
// By pressing the button, the state of the bubble will be toggled.
class CollapseButton : public views::ImageButton {
 public:
  CollapseButton(views::ButtonListener* listener);
  ~CollapseButton() override;

  // Change the icon for the |expanded| state.
  void UpdateIcon(bool expanded);

  // views::ImageButton:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;
  void PaintButtonContents(gfx::Canvas* canvas) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CollapseButton);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_COLLAPSE_BUTTON_H_
