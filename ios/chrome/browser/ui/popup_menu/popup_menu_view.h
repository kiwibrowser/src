// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_VIEW_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_VIEW_H_

#import <UIKit/UIKit.h>

// A protocol required by delegates of the PopupMenuController.
@protocol PopupMenuViewDelegate
// Instructs the delegate the popup menu view is done and should be
// dismissed.
- (void)dismissPopupMenu;
@end

// TODO(crbug.com/800266): Remove this class.
@interface PopupMenuView : UIView
@property(nonatomic, weak) id<PopupMenuViewDelegate> delegate;
@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_VIEW_H_
