// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_H_

#import <Cocoa/Cocoa.h>

// Protocol for a sheet to be showing using |ConstrainedWindowSheetController|.
@protocol ConstrainedWindowSheet<NSObject>

- (void)showSheetForWindow:(NSWindow*)window;

- (void)closeSheetWithAnimation:(BOOL)withAnimation;

- (void)hideSheet;

- (void)unhideSheet;

- (void)pulseSheet;

- (void)makeSheetKeyAndOrderFront;

- (void)updateSheetPosition;

- (void)resizeWithNewSize:(NSSize)size;

@property(readonly, nonatomic) NSWindow* sheetWindow;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_H_
