// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SIDE_SWIPE_SIDE_SWIPE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_SIDE_SWIPE_SIDE_SWIPE_UTIL_H_

#import <UIKit/UIKit.h>

// If swiping to the right (or left in RTL).
BOOL IsSwipingBack(UISwipeGestureRecognizerDirection direction);

// If swiping to the left (or right in RTL).
BOOL IsSwipingForward(UISwipeGestureRecognizerDirection direction);

#endif  // IOS_CHROME_BROWSER_UI_SIDE_SWIPE_SIDE_SWIPE_UTIL_H_
