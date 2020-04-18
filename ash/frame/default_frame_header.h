// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_DEFAULT_FRAME_HEADER_H_
#define ASH_FRAME_DEFAULT_FRAME_HEADER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/frame/frame_header.h"
#include "base/compiler_specific.h"  // override
#include "base/gtest_prod_util.h"
#include "base/macros.h"

namespace ash {

// Helper class for managing the default window header, which is used for
// Chrome apps (but not bookmark apps), for example.
class ASH_EXPORT DefaultFrameHeader : public FrameHeader {
 public:
  // DefaultFrameHeader does not take ownership of any of the parameters.
  DefaultFrameHeader(views::Widget* target_widget,
                     views::View* header_view,
                     FrameCaptionButtonContainerView* caption_button_container);
  ~DefaultFrameHeader() override;

  void SetThemeColor(SkColor theme_color);

  SkColor active_frame_color_for_testing() { return active_frame_color_; }
  SkColor inactive_frame_color_for_testing() { return inactive_frame_color_; }

 protected:
  // FrameHeader:
  void DoPaintHeader(gfx::Canvas* canvas) override;
  void DoSetFrameColors(SkColor active_frame_color,
                        SkColor inactive_frame_color) override;
  AshLayoutSize GetButtonLayoutSize() const override;
  SkColor GetTitleColor() const override;
  SkColor GetCurrentFrameColor() const override;
  void SetWidthInPixels(int width_in_pixels) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(DefaultFrameHeaderTest, FrameColors);

  // Updates the frame colors and ensures buttons are up to date.
  void SetFrameColorsImpl(SkColor active_frame_color,
                          SkColor inactive_frame_color);

  SkColor active_frame_color_;
  SkColor inactive_frame_color_;

  int width_in_pixels_ = -1;

  DISALLOW_COPY_AND_ASSIGN(DefaultFrameHeader);
};

}  // namespace ash

#endif  // ASH_FRAME_DEFAULT_FRAME_HEADER_H_
