// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_SHEET_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_SHEET_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"

// Represents a custom sheet. The sheet's window is shown without using the
// system |beginSheet:...| API.
@interface CustomConstrainedWindowSheet : NSObject<ConstrainedWindowSheet> {
 @protected
  base::scoped_nsobject<NSWindow> customWindow_;
  BOOL useSimpleAnimations_;
}

- (id)initWithCustomWindow:(NSWindow*)customWindow;

// Defaults to NO, unless hardware that has problems with the standard
// animations is detected.
// The standard animation uses private CGS APIs, which can crash the window
// server. https://crbug.com/515627#c75
- (void)setUseSimpleAnimations:(BOOL)simpleAnimations;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_SHEET_H_
