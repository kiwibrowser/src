// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/ui/caption_bar.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Appearance.
constexpr int kCaptionButtonSizeDip = 12;
constexpr int kPaddingDip = 14;
constexpr int kPreferredHeightDip = 48;

// CaptionButton ---------------------------------------------------------------

class CaptionButton : public views::ImageButton {
 public:
  explicit CaptionButton(views::ButtonListener* listener)
      : views::ImageButton(listener) {
    // Currently there is only a single caption button for close. We can easily
    // parameterize the icon to support additional caption buttons if need be.
    SetImage(views::Button::ButtonState::STATE_NORMAL,
             gfx::CreateVectorIcon(kWindowControlCloseIcon,
                                   kCaptionButtonSizeDip, gfx::kGoogleGrey700));
  }

  ~CaptionButton() override = default;

  // views::View:
  gfx::Size CalculatePreferredSize() const override {
    return gfx::Size(kCaptionButtonSizeDip, kCaptionButtonSizeDip);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CaptionButton);
};

}  // namespace

// CaptionBar ------------------------------------------------------------------

CaptionBar::CaptionBar() {
  InitLayout();
}

CaptionBar::~CaptionBar() = default;

gfx::Size CaptionBar::CalculatePreferredSize() const {
  return gfx::Size(INT_MAX, GetHeightForWidth(INT_MAX));
}

int CaptionBar::GetHeightForWidth(int width) const {
  return kPreferredHeightDip;
}

void CaptionBar::InitLayout() {
  views::BoxLayout* layout_manager =
      SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(0, kPaddingDip)));

  layout_manager->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::CROSS_AXIS_ALIGNMENT_CENTER);

  layout_manager->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::MAIN_AXIS_ALIGNMENT_END);

  // Close.
  AddChildView(new CaptionButton(this));
}

void CaptionBar::ButtonPressed(views::Button* sender, const ui::Event& event) {
  // There is currently only a single caption button for close.
  GetWidget()->Close();
}

}  // namespace ash
