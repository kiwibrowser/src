// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_NSMENUITEM_ADDITIONS_H_
#define CHROME_BROWSER_UI_COCOA_NSMENUITEM_ADDITIONS_H_

#import <Cocoa/Cocoa.h>

@interface NSMenuItem(ChromeAdditions)

// Returns true exactly if the menu item would fire if it would be put into
// a menu and then |menu performKeyEquivalent:event| was called.
// This method always returns NO if the menu item is not enabled.
- (BOOL)cr_firesForKeyEvent:(NSEvent*)event;

// Like above method, but this method matches the key equivalent regardless of
// the menu item's enable state.
- (BOOL)cr_firesForKeyEventIfEnabled:(NSEvent*)event;

@end

#endif  // CHROME_BROWSER_UI_COCOA_NSMENUITEM_ADDITIONS_H_
