// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_MENU_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_MENU_BUTTON_H_

#include "base/timer/timer.h"
#include "chrome/browser/ui/views/frame/app_menu_button.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/menu_button_listener.h"

class BrowserView;

namespace base {
class TimeDelta;
}

// The 'app menu' button for the hosted app.
class HostedAppMenuButton : public AppMenuButton,
                            public views::MenuButtonListener {
 public:
  explicit HostedAppMenuButton(BrowserView* browser_view);
  ~HostedAppMenuButton() override;

  // Sets the color of the menu button icon.
  void SetIconColor(SkColor color);

  // Fades the menu button highlight on and off.
  void StartHighlightAnimation(base::TimeDelta duration);

  // views::MenuButtonListener:
  void OnMenuButtonClicked(views::MenuButton* source,
                           const gfx::Point& point,
                           const ui::Event* event) override;

 private:
  void FadeHighlightOff();

  // The containing browser view.
  BrowserView* browser_view_;

  base::OneShotTimer highlight_off_timer_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppMenuButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_MENU_BUTTON_H_
