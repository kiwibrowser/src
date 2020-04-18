// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_LAYOUT_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_LAYOUT_H_

#import <UIKit/UIKit.h>

// Collection view flow layout that displays items in a grid. Items are
// square-ish. Item sizes adapt to the size classes they are shown in. Item
// deletions are animated.
@interface GridLayout : UICollectionViewFlowLayout
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_LAYOUT_H_
