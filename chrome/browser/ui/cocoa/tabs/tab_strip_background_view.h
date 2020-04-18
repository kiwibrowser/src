// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_BACKGROUND_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_BACKGROUND_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/themed_window.h"

// A view that draws the theme image in the top area of the window (behind the
// tab strip area). It should be arranged so that its z-order is below its
// overlapping sibling views (window controls, tab strip view, profile button
// and fullscreen button).
@interface TabStripBackgroundView : NSView<ThemedWindowDrawing>
@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_BACKGROUND_VIEW_H_
