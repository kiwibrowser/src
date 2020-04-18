// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FLOATING_BAR_BACKING_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_FLOATING_BAR_BACKING_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/themed_window.h"

// A custom view that draws the tab strip background for fullscreen windows.
@interface FloatingBarBackingView : NSView<ThemedWindowDrawing>
@end

#endif  // CHROME_BROWSER_UI_COCOA_FLOATING_BAR_BACKING_VIEW_H_
