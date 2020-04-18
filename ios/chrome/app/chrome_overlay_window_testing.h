// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_TESTING_H_
#define IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_TESTING_H_

#import "ios/chrome/app/chrome_overlay_window.h"

@interface ChromeOverlayWindow (ForTesting)

// Cleans up the size class recorder of main window for testing.
- (void)unsetSizeClassRecorder;

@end
#endif  // IOS_CHROME_APP_CHROME_OVERLAY_WINDOW_TESTING_H_
