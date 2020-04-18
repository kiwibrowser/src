// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_STYLER_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_STYLER_H_

#import <UIKit/UIKit.h>

@interface ChromeTableViewStyler : NSObject

// The background color for the table view and its cells. If this is set to an
// opaque color, cells can choose to make themselves opaque and draw their own
// background as a performance optimization.
@property(nonatomic, readwrite, strong) UIColor* tableViewBackgroundColor;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_STYLER_H_
