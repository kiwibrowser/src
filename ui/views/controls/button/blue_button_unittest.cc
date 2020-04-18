// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/blue_button.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "cc/paint/skia_paint_canvas.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/test/widget_test.h"

namespace views {

using BlueButtonTest = test::WidgetTest;

TEST_F(BlueButtonTest, Border) {
  // The buttons must be added to a Widget so that borders are correctly
  // applied once the NativeTheme is determined.
  Widget* widget = CreateTopLevelPlatformWidget();

  // Compared to a normal LabelButton...
  LabelButton* button = new LabelButton(nullptr, base::ASCIIToUTF16("foo"));
  EXPECT_EQ(Button::STYLE_TEXTBUTTON, button->style());
  // Focus painter by default.
  EXPECT_TRUE(button->focus_painter_.get());

  // Switch to the same style as BlueButton for a more compelling comparison.
  button->SetStyleDeprecated(Button::STYLE_BUTTON);
  EXPECT_EQ(Button::STYLE_BUTTON, button->style());
  EXPECT_FALSE(button->focus_painter_.get());

  widget->GetContentsView()->AddChildView(button);
  button->SizeToPreferredSize();

  SkBitmap button_bitmap;
  button_bitmap.allocN32Pixels(button->size().width(), button->size().height(),
                               true /* opaque */);
  cc::SkiaPaintCanvas button_paint_canvas(button_bitmap);
  gfx::Canvas button_canvas(&button_paint_canvas, 1.f);
  button->border()->Paint(*button, &button_canvas);

  // ... a special blue border should be used.
  BlueButton* blue_button = new BlueButton(nullptr, base::ASCIIToUTF16("foo"));
  EXPECT_EQ(Button::STYLE_BUTTON, blue_button->style());
  EXPECT_FALSE(blue_button->focus_painter_.get());

  widget->GetContentsView()->AddChildView(blue_button);
  blue_button->SizeToPreferredSize();

  SkBitmap blue_button_bitmap;
  blue_button_bitmap.allocN32Pixels(blue_button->size().width(),
                                    blue_button->size().height(),
                                    true /* opaque */);
  cc::SkiaPaintCanvas blue_button_paint_canvas(blue_button_bitmap);
  gfx::Canvas blue_button_canvas(&blue_button_paint_canvas, 1.f);
  blue_button->border()->Paint(*blue_button, &blue_button_canvas);
  EXPECT_EQ(button->GetText(), blue_button->GetText());
  EXPECT_EQ(button->size(), blue_button->size());
  EXPECT_FALSE(gfx::BitmapsAreEqual(button_bitmap, blue_button_bitmap));

  widget->CloseNow();
}

}  // namespace views
