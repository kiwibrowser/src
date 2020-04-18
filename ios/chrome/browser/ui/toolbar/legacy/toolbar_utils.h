// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UTILS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UTILS_H_

#import <UIKit/UIKit.h>

// Returns the total height of the toolbar by taking into account the
// |status_bar_offset| which needs to be added.
CGFloat ToolbarHeightWithTopOfScreenOffset(CGFloat status_bar_offset);

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UTILS_H_
