// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"

@class TabSwitcherHeaderView;
@class TabSwitcherView;

enum class NewTabButtonStyle { UNINITIALIZED, BLUE, GRAY, HIDDEN };

@protocol TabSwitcherViewDelegate<NSObject>

- (void)openNewTabInPanelAtIndex:(NSInteger)panelIndex;
- (NewTabButtonStyle)buttonStyleForPanelAtIndex:(NSInteger)panelIndex;
- (BOOL)shouldShowDismissButtonForPanelAtIndex:(NSInteger)panelIndex;
- (void)tabSwitcherViewDelegateDismissTabSwitcher:(TabSwitcherView*)delegate;

@end

@interface TabSwitcherView : UIView<UIScrollViewDelegate>

@property(nonatomic, readonly, weak) TabSwitcherHeaderView* headerView;
@property(nonatomic, readonly, weak) UIScrollView* scrollView;

@property(nonatomic, weak) id<TabSwitcherViewDelegate> delegate;

// Select the panel at the given index, updating both the header and content.
// The panel selection will be animated if VoiceOver is disabled.
- (void)selectPanelAtIndex:(NSInteger)index;
// Adds the view |view| at index |index|.
- (void)addPanelView:(UIView*)view atIndex:(NSUInteger)index;
// Removes the view at index |index| and update scrollview.
- (void)removePanelViewAtIndex:(NSUInteger)index;
// Removes the view at index |index| and update scrollview if |update| is true.
- (void)removePanelViewAtIndex:(NSUInteger)index updateScrollView:(BOOL)update;
// Returns the index of the currently selected panel.
- (NSInteger)currentPanelIndex;
// Updates the state of the tab switcher overlay buttons.
// Overlay buttons are the top right dismiss button and the bottom right new tab
// button.
- (void)updateOverlayButtonState;
// Should be called when the tab switcher was shown.
- (void)wasShown;
// Should be called when the tab switcher was hidden.
- (void)wasHidden;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_VIEW_H_
