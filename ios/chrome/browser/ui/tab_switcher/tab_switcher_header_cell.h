// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_CELL_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_CELL_H_

#import <UIKit/UIKit.h>

@class TabSwitcherSessionCellData;

// This class is the cell class used in the table view of the tab switcher
// header.
@interface TabSwitcherHeaderCell : UICollectionViewCell

// Default table view cell identifier.
+ (NSString*)identifier;

// Load the cell content using the given TabSwitcherSessionCellData object.
- (void)loadSessionCellData:(TabSwitcherSessionCellData*)sessionCellData;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_CELL_H_
