// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/default_theme_provider.h"

#import <Cocoa/Cocoa.h>

#include "ui/base/resource/resource_bundle.h"

namespace ui {

bool DefaultThemeProvider::UsingSystemTheme() const {
  return true;
}

bool DefaultThemeProvider::InIncognitoMode() const {
  return false;
}

bool DefaultThemeProvider::HasCustomColor(int id) const {
  return false;
}

NSImage* DefaultThemeProvider::GetNSImageNamed(int id) const {
 return ResourceBundle::GetSharedInstance().
     GetNativeImageNamed(id).ToNSImage();
}

NSColor* DefaultThemeProvider::GetNSImageColorNamed(int id) const {
  NSImage* image = GetNSImageNamed(id);
  return [NSColor colorWithPatternImage:image];
}

NSColor* DefaultThemeProvider::GetNSColor(int id) const {
  return [NSColor redColor];
}

NSColor* DefaultThemeProvider::GetNSColorTint(int id) const {
  return [NSColor redColor];
}

NSGradient* DefaultThemeProvider::GetNSGradient(int id) const {
  return nil;
}

bool DefaultThemeProvider::ShouldIncreaseContrast() const {
  return false;
}

}  // namespace ui
