// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_

#import <Cocoa/Cocoa.h>

#include "ui/base/cocoa/touch_bar_forward_declarations.h"

@interface SeparateFullscreenWindow : NSWindow
// This window class is instantiated to display a WebContentsViewCocoa as its
// subview in fullscreen mode as part of ContentFullscreen to enable users to
// interact with the main browser window and its tab when they're displaying
// content in this separate window. Related types include
// FullscreenLowPowerWindow.
@end

#endif  // CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
