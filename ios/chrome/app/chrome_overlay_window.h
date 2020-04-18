// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_H_
#define IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_H_

#import "ios/third_party/material_components_ios/src/components/OverlayWindow/src/MaterialOverlayWindow.h"

// Tracks size classes changes then reports to SizeClassRecorder and Breakpad.
@interface ChromeOverlayWindow : MDCOverlayWindow
@end

#endif  // IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_H_
