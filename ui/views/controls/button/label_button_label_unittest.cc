// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/label_button_label.h"

#include <memory>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/native_theme/native_theme_base.h"
#include "ui/views/test/views_test_base.h"

namespace views {
namespace {

// Basic NativeTheme that can customize colors.
class TestNativeTheme : public ui::NativeThemeBase {
 public:
  TestNativeTheme() {}

  void Set(ColorId id, SkColor color) { colors_[id] = color; }

  // NativeThemeBase:
  SkColor GetSystemColor(ColorId color_id) const override {
    return colors_.count(color_id) ? colors_.find(color_id)->second
                                   : SK_ColorMAGENTA;
  }

 private:
  std::map<ColorId, SkColor> colors_;

  DISALLOW_COPY_AND_ASSIGN(TestNativeTheme);
};

// LabelButtonLabel subclass that reports its text color whenever a paint is
// scheduled.
class TestLabel : public LabelButtonLabel {
 public:
  explicit TestLabel(SkColor* last_color)
      : LabelButtonLabel(base::string16(), views::style::CONTEXT_BUTTON),
        last_color_(last_color) {}

  // LabelButtonLabel:
  void SchedulePaintInRect(const gfx::Rect& r) override {
    LabelButtonLabel::SchedulePaintInRect(r);
    *last_color_ = enabled_color();
  }

 private:
  SkColor* last_color_;

  DISALLOW_COPY_AND_ASSIGN(TestLabel);
};

}  // namespace

class LabelButtonLabelTest : public ViewsTestBase {
 public:
  LabelButtonLabelTest() {}

  void SetUp() override {
    ViewsTestBase::SetUp();
    label_ = std::make_unique<TestLabel>(&last_color_);
  }

 protected:
  SkColor last_color_ = SK_ColorCYAN;
  std::unique_ptr<TestLabel> label_;
  TestNativeTheme theme1_;
  TestNativeTheme theme2_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LabelButtonLabelTest);
};

// Test that LabelButtonLabel reacts properly to themed and overridden colors.
TEST_F(LabelButtonLabelTest, Colors) {
  // The SchedulePaintInRect override won't be called while the base class is
  // initialized. Not much we can do about that, so give it the first for free.
  EXPECT_EQ(SK_ColorCYAN, last_color_);  // Sanity check.

  // At the same time we can check that changing the auto color readability
  // schedules a paint. Currently it does. Although it technically doesn't need
  // to since the color isn't actually changing.
  label_->SetAutoColorReadabilityEnabled(false);

  // First one comes from the default theme. This check ensures the SK_ColorRED
  // placeholder initializers were replaced.
  SkColor default_theme_enabled_color =
      label_->GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelEnabledColor);
  EXPECT_EQ(default_theme_enabled_color, last_color_);

  // Note these are not kColorId_Button{Enabled,Disabled}Color because "Button
  // colors are used only for STYLE_BUTTON, otherwise we use label colors." See
  // LabelButton::ResetColorsFromNativeTheme().
  theme1_.Set(ui::NativeTheme::kColorId_LabelEnabledColor, SK_ColorGREEN);
  theme1_.Set(ui::NativeTheme::kColorId_LabelDisabledColor, SK_ColorYELLOW);
  label_->SetNativeTheme(&theme1_);

  // Setting the theme should paint.
  EXPECT_EQ(SK_ColorGREEN, last_color_);

  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorYELLOW, last_color_);

  // Set up a second theme. View::SetNativeTheme() makes the reasonable
  // assumption that NativeTheme doesn't change its mind about things unless a
  // Widget triggers it (which it can do as a friend of RootView).
  theme2_.Set(ui::NativeTheme::kColorId_LabelEnabledColor, SK_ColorBLUE);
  theme2_.Set(ui::NativeTheme::kColorId_LabelDisabledColor, SK_ColorGRAY);
  label_->SetNativeTheme(&theme2_);

  EXPECT_EQ(SK_ColorGRAY, last_color_);

  label_->SetEnabled(true);
  EXPECT_EQ(SK_ColorBLUE, last_color_);

  // Override the theme for the disabled color.
  label_->SetDisabledColor(SK_ColorRED);

  // Still enabled, so not RED yet.
  EXPECT_EQ(SK_ColorBLUE, last_color_);

  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorRED, last_color_);

  label_->SetDisabledColor(SK_ColorMAGENTA);
  EXPECT_EQ(SK_ColorMAGENTA, last_color_);

  // Disabled still overridden after a theme change.
  label_->SetNativeTheme(&theme1_);
  EXPECT_EQ(SK_ColorMAGENTA, last_color_);

  // The enabled color still gets its value from the theme.
  label_->SetEnabled(true);
  EXPECT_EQ(SK_ColorGREEN, last_color_);

  label_->SetEnabledColor(SK_ColorYELLOW);
  label_->SetDisabledColor(SK_ColorCYAN);
  EXPECT_EQ(SK_ColorYELLOW, last_color_);
  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorCYAN, last_color_);
}

}  // namespace views
