// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/download/background_theme.h"

#import "chrome/browser/themes/theme_properties.h"
#include "ui/gfx/color_utils.h"

BackgroundTheme::BackgroundTheme(const ui::ThemeProvider* provider)
    : provider_(provider) {
  NSColor* bgColor = [NSColor colorWithCalibratedRed:241/255.0
                                               green:245/255.0
                                                blue:250/255.0
                                               alpha:77/255.0];
  NSColor* clickedColor = [NSColor colorWithCalibratedRed:239/255.0
                                                    green:245/255.0
                                                     blue:252/255.0
                                                    alpha:51/255.0];

  borderColor_.reset(
      [[NSColor colorWithCalibratedWhite:0 alpha:36/255.0] retain]);
  buttonGradient_.reset([[NSGradient alloc]
      initWithColors:[NSArray arrayWithObject:bgColor]]);
  buttonPressedGradient_.reset([[NSGradient alloc]
      initWithColors:[NSArray arrayWithObject:clickedColor]]);
}

BackgroundTheme::~BackgroundTheme() {}

bool BackgroundTheme::UsingSystemTheme() const {
  return true;
}

bool BackgroundTheme::InIncognitoMode() const {
  return false;
}

bool BackgroundTheme::HasCustomColor(int id) const {
  return false;
}

gfx::ImageSkia* BackgroundTheme::GetImageSkiaNamed(int id) const {
  return NULL;
}

SkColor BackgroundTheme::GetColor(int id) const {
  return SkColor();
}

color_utils::HSL BackgroundTheme::GetTint(int id) const {
  return color_utils::HSL();
}

int BackgroundTheme::GetDisplayProperty(int id) const {
  return -1;
}

bool BackgroundTheme::ShouldUseNativeFrame() const {
  return false;
}

bool BackgroundTheme::HasCustomImage(int id) const {
  return false;
}

base::RefCountedMemory* BackgroundTheme::GetRawData(
    int id,
    ui::ScaleFactor scale_factor) const {
  return NULL;
}

NSImage* BackgroundTheme::GetNSImageNamed(int id) const {
  return nil;
}

NSColor* BackgroundTheme::GetNSImageColorNamed(int id) const {
  return nil;
}

NSColor* BackgroundTheme::GetNSColor(int id) const {
  return provider_->GetNSColor(id);
}

NSColor* BackgroundTheme::GetNSColorTint(int id) const {
  if (id == ThemeProperties::TINT_BUTTONS)
    return borderColor_.get();

  return provider_->GetNSColorTint(id);
}

NSGradient* BackgroundTheme::GetNSGradient(int id) const {
  switch (id) {
    case ThemeProperties::GRADIENT_TOOLBAR_BUTTON:
    case ThemeProperties::GRADIENT_TOOLBAR_BUTTON_INACTIVE:
      return buttonGradient_.get();
    case ThemeProperties::GRADIENT_TOOLBAR_BUTTON_PRESSED:
    case ThemeProperties::GRADIENT_TOOLBAR_BUTTON_PRESSED_INACTIVE:
      return buttonPressedGradient_.get();
    default:
      return provider_->GetNSGradient(id);
  }
}

bool BackgroundTheme::ShouldIncreaseContrast() const {
  return false;
}
