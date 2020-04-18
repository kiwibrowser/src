// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/chrome_browser_window.h"

#include "base/logging.h"
#include "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "ui/base/theme_provider.h"

namespace {

// Upper and lower bounds for determining if a theme's colors indicate that
// it's a "dark" theme. In Material Design, dark themes have controls that are
// drawn using transparent white instead of a transparent shade of gray.
const CGFloat kDarkThemeToolbarColorUpperBound = 0.55;
const CGFloat kDarkThemeTabTextColorLowerBound = 0.7;

}  // namespace

@interface NSWindow (Private)
- (BOOL)hasKeyAppearance;
@end

@implementation ChromeBrowserWindow

- (const ui::ThemeProvider*)themeProvider {
  id tabWindowController =
      [TabWindowController tabWindowControllerForWindow:self];
  if (![tabWindowController respondsToSelector:@selector(themeProvider)])
    return NULL;
  return [tabWindowController themeProvider];
}

- (ThemedWindowStyle)themedWindowStyle {
  id tabWindowController =
      [TabWindowController tabWindowControllerForWindow:self];
  if (![tabWindowController respondsToSelector:@selector(themedWindowStyle)])
    return THEMED_NORMAL;
  return [tabWindowController themedWindowStyle];
}

- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment {
  id tabWindowController =
      [TabWindowController tabWindowControllerForWindow:self];
  if (![tabWindowController
          respondsToSelector:@selector(themeImagePositionForAlignment:)])
    return NSZeroPoint;
  return [tabWindowController themeImagePositionForAlignment:alignment];
}

- (BOOL)inIncognitoMode {
  const ui::ThemeProvider* themeProvider = [self themeProvider];
  return themeProvider && themeProvider->InIncognitoMode();
}

- (BOOL)inIncognitoModeWithSystemTheme {
  const ui::ThemeProvider* themeProvider = [self themeProvider];
  return themeProvider && themeProvider->InIncognitoMode() &&
      themeProvider->UsingSystemTheme();
}

- (BOOL)hasDarkTheme {
  // If a system theme, return YES if Incognito.
  const ui::ThemeProvider* themeProvider = [self themeProvider];
  if (!themeProvider) {
    return NO;
  } else if (themeProvider->UsingSystemTheme()) {
    return themeProvider->InIncognitoMode();
  }

  // If the custom theme has a custom toolbar color, return YES if it's
  // dark.
  if (themeProvider->HasCustomColor(ThemeProperties::COLOR_TOOLBAR)) {
    NSColor* theColor =
        themeProvider->GetNSColor(ThemeProperties::COLOR_TOOLBAR);
    theColor =
        [theColor colorUsingColorSpaceName:NSCalibratedWhiteColorSpace];
    if (theColor != nil) {
      // The white componement cutoff is an empirical value.
      return [theColor whiteComponent] < kDarkThemeToolbarColorUpperBound;
    }
  }

  // If the custom theme has a custom tab text color, assume that a light
  // color means a dark tab background image, and therefore a dark theme.
  if (themeProvider->HasCustomColor(ThemeProperties::COLOR_TAB_TEXT)) {
    NSColor* theColor =
        themeProvider->GetNSColor(ThemeProperties::COLOR_TAB_TEXT);
    theColor =
        [theColor colorUsingColorSpaceName:NSCalibratedWhiteColorSpace];
    if (theColor != nil) {
      return [theColor whiteComponent] >= kDarkThemeTabTextColorLowerBound;
    }
  }

  return NO;
}

- (BOOL)hasKeyAppearance {
  // If not key, but a non-main child window without its own traffic lights _is_
  // key, then show this window with key appearance to keep the traffic lights
  // lit. This does not currently handle WebModal dialogs, since they are
  // children of an overlay window. But WebModals also temporarily lose key
  // status while animating closed, so extra logic is needed to avoid flicker.
  // Start with an early exit, since this is called for every mouseMove and
  // every cursor blink in an NSTextField.
  if (![self isKeyWindow]) {
    for (NSWindow* child in [self childWindows]) {
      if ([child isKeyWindow] && ![child isMainWindow] &&
          ([child styleMask] & NSClosableWindowMask) == 0)
        return YES;
    }
  }
  return [super hasKeyAppearance];
}

@end
