// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_UI_CAPTION_BAR_H_
#define ASH_ASSISTANT_UI_CAPTION_BAR_H_

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace ash {

class CaptionBar : public views::View, views::ButtonListener {
 public:
  CaptionBar();
  ~CaptionBar() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  void InitLayout();

  DISALLOW_COPY_AND_ASSIGN(CaptionBar);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_UI_CAPTION_BAR_H_
