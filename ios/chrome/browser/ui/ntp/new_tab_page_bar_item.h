// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_ITEM_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_ITEM_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"

// Represents an item on the new tab page bar, similar to a UITabBarItem.
@interface NewTabPageBarItem : NSObject

// Convenience method for creating a tab bar choice.
+ (NewTabPageBarItem*)newTabPageBarItemWithTitle:(NSString*)title
                                      identifier:
                                          (ntp_home::PanelIdentifier)identifier
                                           image:(UIImage*)imageName
    NS_RETURNS_NOT_RETAINED;

@property(nonatomic, copy) NSString* title;
@property(nonatomic, assign) ntp_home::PanelIdentifier identifier;
@property(nonatomic, strong) UIImage* image;
@property(nonatomic, weak) UIView* view;
@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_ITEM_H_
