// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_CAPTION_BUTTONS_FRAME_BACK_BUTTON_H_
#define ASH_FRAME_CAPTION_BUTTONS_FRAME_BACK_BUTTON_H_

#include "ash/ash_export.h"
#include "ash/frame/caption_buttons/frame_caption_button.h"

namespace ash {

// A button to send back key events.
class ASH_EXPORT FrameBackButton : public FrameCaptionButton,
                                   public views::ButtonListener {
 public:
  FrameBackButton();
  ~FrameBackButton() override;

  // views::ButtonListener:
  void ButtonPressed(Button* sender, const ui::Event& event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FrameBackButton);
};

}  // namespace ash

#endif  //  ASH_FRAME_CAPTION_BUTTONS_FRAME_BACK_BUTTON_H_
