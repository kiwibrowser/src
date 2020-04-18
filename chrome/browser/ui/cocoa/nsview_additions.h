// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_NSVIEW_ADDITIONS_H_
#define CHROME_BROWSER_UI_COCOA_NSVIEW_ADDITIONS_H_

#import <AppKit/AppKit.h>

@interface NSView (ChromeBrowserAdditions)

// If the UI is in RTL mode, swaps NSViewMinXMargin and NSViewMaxXMargin.
+ (NSAutoresizingMaskOptions)cr_localizedAutoresizingMask:
    (NSAutoresizingMaskOptions)mask;

// If the UI is in RTL mode, flips the rect in the receiver's bounds.
- (NSRect)cr_localizedRect:(NSRect)rect;

@end

#endif  // CHROME_BROWSER_UI_COCOA_NSVIEW_ADDITIONS_H_
