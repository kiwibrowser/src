// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_PANEL_BAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_PANEL_BAR_VIEW_H_

#import <UIKit/UIKit.h>

// View for the bar located at the top of the Recent Tabs panel.
@interface PanelBarView : UIView

// Designated initializer.
- (instancetype)init;

// Sets the target/action of the close button.
- (void)setCloseTarget:(id)target action:(SEL)action;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_PANEL_BAR_VIEW_H_
