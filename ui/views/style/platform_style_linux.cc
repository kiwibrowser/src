// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/style/platform_style.h"

#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/linux_ui/linux_ui.h"

namespace views {

// static
std::unique_ptr<Border> PlatformStyle::CreateThemedLabelButtonBorder(
    LabelButton* button) {
  views::LinuxUI* linux_ui = views::LinuxUI::instance();
  if (linux_ui)
    return linux_ui->CreateNativeBorder(button, button->CreateDefaultBorder());
  return button->CreateDefaultBorder();
}

void PlatformStyle::ApplyLabelButtonTextStyle(Label* label,
                                              ButtonColorByState* colors) {
  // TODO(erg): This is disabled on desktop linux because of the binary asset
  // confusion. These details should either be pushed into ui::NativeThemeWin
  // or should be obsoleted by rendering buttons with paint calls instead of
  // with static assets. http://crbug.com/350498
}

}  // namespace views
