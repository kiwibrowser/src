// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/themed_window.h"

// Default implementations; used mostly for tests so that the hosting windows
// don't needs to know about the theming machinery.
@implementation NSWindow (ThemeProvider)

- (const ThemeProvider*)themeProvider {
  return NULL;
}

- (ThemedWindowStyle)themedWindowStyle {
  return THEMED_NORMAL;
}

- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment {
  return NSZeroPoint;
}

- (BOOL)inIncognitoMode {
  return NO;
}

- (BOOL)inIncognitoModeWithSystemTheme {
  return NO;
}

- (BOOL)hasDarkTheme {
  return NO;
}

@end
