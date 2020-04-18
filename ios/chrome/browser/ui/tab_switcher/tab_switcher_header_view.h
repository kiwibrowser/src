// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"

@class TabSwitcherSessionCellData;
@class TabSwitcherHeaderView;

@protocol TabSwitcherHeaderViewDelegate<NSObject>

- (void)tabSwitcherHeaderViewDismiss:(TabSwitcherHeaderView*)view;
- (void)tabSwitcherHeaderViewDidSelectSessionAtIndex:(NSInteger)index;
// Called to get the currently selected panel index. Returns NSNotFound when no
// panel is selected.
- (NSInteger)tabSwitcherHeaderViewSelectedPanelIndex;

@end

@protocol TabSwitcherHeaderViewDataSource<NSObject>

- (NSInteger)tabSwitcherHeaderViewSessionCount;
- (TabSwitcherSessionCellData*)sessionCellDataAtIndex:(NSUInteger)index;

@end

@interface TabSwitcherHeaderView : UIView

@property(nonatomic, weak) id<TabSwitcherHeaderViewDelegate> delegate;
@property(nonatomic, weak) id<TabSwitcherHeaderViewDataSource> dataSource;
@property(weak, nonatomic, readonly) UIView* dismissButton;

// Selects the item at the specified index.
// The delegate is not called.
- (void)selectItemAtIndex:(NSInteger)index;

// Reload the collection view.
- (void)reloadData;

// Performs an update on the header view using the passed block.
- (void)performUpdate:(void (^)(TabSwitcherHeaderView* headerView))updateBlock;

// Performs an update on the header view using the passed block, the completion
// handler block is called at the end of the update animations.
- (void)performUpdate:(void (^)(TabSwitcherHeaderView* headerView))updateBlock
           completion:(ProceduralBlock)completion;

// Updates methods can only be called inside an update block passed to the
// - (void)performUpdate: method.
// Update methods takes arrays of NSNumbers.
- (void)insertSessionsAtIndexes:(NSArray*)indexes;
- (void)removeSessionsAtIndexes:(NSArray*)indexes;

// Returns the accessibility title of the panel at index |index|.
- (NSString*)panelTitleAtIndex:(NSInteger)index;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_HEADER_VIEW_H_
