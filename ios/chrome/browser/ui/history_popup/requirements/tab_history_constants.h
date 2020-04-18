// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_CONSTANTS_H_

#import <Foundation/Foundation.h>

// Notification center names.
extern NSString* const kTabHistoryPopupWillShowNotification;
extern NSString* const kTabHistoryPopupWillHideNotification;

// Toolbar buttons that can display the HistoryPopup.
typedef NS_OPTIONS(NSUInteger, ToolbarButtonType) {
  // Back Toolbar Button.
  ToolbarButtonTypeForward = 0,
  // Forward Toolbar Button.
  ToolbarButtonTypeBack = 1,
};

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_CONSTANTS_H_
