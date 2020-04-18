// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_FAVICON_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_FAVICON_VIEW_H_

#import "chrome/browser/ui/cocoa/tabs/tab_spinner_view.h"

#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"

@interface TabFaviconView : NSView<ThemedWindowDrawing>

// Accessor for the tab's loading state. Use setTabDoneIcon: to set the
// icon and loading state to kTabDone.
@property(assign, nonatomic) TabLoadingState tabLoadingState;

// Sets the tab's icon and loading state to kTabDone.
- (void)setTabDoneStateWithIcon:(NSImage*)anImage;

@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_FAVICON_VIEW_H_
