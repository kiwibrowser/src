// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_BACKGROUND_THEME_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_BACKGROUND_THEME_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "ui/base/theme_provider.h"

class BackgroundTheme : public ui::ThemeProvider {
 public:
  BackgroundTheme(const ui::ThemeProvider* provider);
  ~BackgroundTheme() override;

  // Overridden from ui::ThemeProvider:
  bool UsingSystemTheme() const override;
  gfx::ImageSkia* GetImageSkiaNamed(int id) const override;
  SkColor GetColor(int id) const override;
  color_utils::HSL GetTint(int id) const override;
  int GetDisplayProperty(int id) const override;
  bool ShouldUseNativeFrame() const override;
  bool HasCustomImage(int id) const override;
  base::RefCountedMemory* GetRawData(int id, ui::ScaleFactor scale_factor)
      const override;
  bool InIncognitoMode() const override;
  bool HasCustomColor(int id) const override;
  NSImage* GetNSImageNamed(int id) const override;
  NSColor* GetNSImageColorNamed(int id) const override;
  NSColor* GetNSColor(int id) const override;
  NSColor* GetNSColorTint(int id) const override;
  NSGradient* GetNSGradient(int id) const override;
  bool ShouldIncreaseContrast() const override;

 private:
  const ui::ThemeProvider* provider_;
  base::scoped_nsobject<NSGradient> buttonGradient_;
  base::scoped_nsobject<NSGradient> buttonPressedGradient_;
  base::scoped_nsobject<NSColor> borderColor_;
};

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_BACKGROUND_THEME_H_
