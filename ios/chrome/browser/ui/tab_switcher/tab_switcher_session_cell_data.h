// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CELL_DATA_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CELL_DATA_H_

#import <UIKit/UIKit.h>

@class TabSwitcherHeaderView;

enum TabSwitcherSessionCellType {
  kIncognitoSessionCell,
  kOpenTabSessionCell,
  kGenericRemoteSessionCell,
  kPhoneRemoteSessionCell,
  kTabletRemoteSessionCell,
  kLaptopRemoteSessionCell,
};

// This class hold the data used to configure the content of a
// TabSwitcherHeaderCell.
@interface TabSwitcherSessionCellData : NSObject

// Those two methods are used to create incognito and open tab cell data.
// Each method create a SessionCellData object on the first call and returns
// the same instance on further calls.
+ (instancetype)incognitoSessionCellData;
+ (instancetype)openTabSessionCellData;
+ (instancetype)otherDevicesSessionCellData;

- (instancetype)initWithSessionCellType:(TabSwitcherSessionCellType)type;

@property(nonatomic, readonly) TabSwitcherSessionCellType type;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, retain) UIImage* image;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CELL_DATA_H_
