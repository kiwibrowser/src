// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_SIGN_OUT_BUTTON_H_
#define ASH_SYSTEM_UNIFIED_SIGN_OUT_BUTTON_H_

#include "ui/views/controls/button/label_button.h"

namespace ash {

// Sign out button shown in TopShortcutView with TopShortcutButtons.
// Shows the label like "Sign out", "Exit guest", etc. depending on the session
// status. Not visible when not signed in.
class SignOutButton : public views::LabelButton {
 public:
  SignOutButton(views::ButtonListener* listener);
  ~SignOutButton() override;

  // views::LabelButton:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;
  void PaintButtonContents(gfx::Canvas* canvas) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SignOutButton);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_SIGN_OUT_BUTTON_H_
