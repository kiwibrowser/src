// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/image_button_factory.h"

#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/animation/test/ink_drop_host_view_test_api.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/test/views_test_base.h"

namespace views {

typedef ViewsTestBase ImageButtonFactoryTest;

TEST_F(ImageButtonFactoryTest, CreateVectorImageButton) {
  ImageButton* button = CreateVectorImageButton(nullptr);
  EXPECT_EQ(ImageButton::ALIGN_CENTER, button->h_alignment_);
  EXPECT_EQ(ImageButton::ALIGN_MIDDLE, button->v_alignment_);
  EXPECT_TRUE(test::InkDropHostViewTestApi(button).HasGestureHandler());
  delete button;
}

TEST_F(ImageButtonFactoryTest, SetImageFromVectorIcon) {
  ImageButton* button = CreateVectorImageButton(nullptr);
  SetImageFromVectorIcon(button, vector_icons::kCloseRoundedIcon, SK_ColorRED);
  EXPECT_FALSE(button->GetImage(Button::STATE_NORMAL).isNull());
  EXPECT_FALSE(button->GetImage(Button::STATE_DISABLED).isNull());
  EXPECT_EQ(color_utils::DeriveDefaultIconColor(SK_ColorRED),
            button->GetInkDropBaseColor());

  // Default to GoogleGrey900.
  SetImageFromVectorIcon(button, vector_icons::kCloseRoundedIcon);
  EXPECT_EQ(color_utils::DeriveDefaultIconColor(gfx::kGoogleGrey900),
            button->GetInkDropBaseColor());
  delete button;
}

}  // views
