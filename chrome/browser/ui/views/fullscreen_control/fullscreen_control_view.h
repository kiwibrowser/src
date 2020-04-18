// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_VIEW_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

// FullscreenControlView shows a FAB (floating action button from the material
// design spec) with close icon (i.e. a partially-transparent black circular
// button with a "X" icon in the middle).
// |on_button_pressed| will be called when the user taps the button.
class FullscreenControlView : public views::View,
                              public views::ButtonListener {
 public:
  explicit FullscreenControlView(
      const base::RepeatingClosure& on_button_pressed);
  ~FullscreenControlView() override;

  static constexpr int kCircleButtonDiameter = 48;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  views::Button* exit_fullscreen_button() { return exit_fullscreen_button_; }

 private:
  const base::RepeatingClosure on_button_pressed_;
  views::Button* const exit_fullscreen_button_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControlView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_VIEW_H_
