// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_
#define IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"

class ReadingListModel;

// Temporary protocol to simplify adding a second header view for the UI
// Refresh. TODO(crbug.com/807330) Remove post UI refresh.
@protocol NTPHeaderViewAdapter<NSObject, ToolbarOwner>

// Return the toolbar view.
@property(nonatomic, readonly) UIView* toolBarView;

// Return the progress of the search field position along
// |ntp_header::kAnimationDistance| as the offset changes.
- (CGFloat)searchFieldProgressForOffset:(CGFloat)offset
                         safeAreaInsets:(UIEdgeInsets)safeAreaInsets;

// Changes the constraints of searchField based on its initialFrame and the
// scroll view's y |offset|. Also adjust the alpha values for |_searchBoxBorder|
// and |_shadow| and the constant values for the |constraints|.|screenWidth| is
// the width of the screen, including the space outside the safe area. The
// |safeAreaInsets| is relative to the view used to calculate the |width|.
- (void)updateSearchFieldWidth:(NSLayoutConstraint*)widthConstraint
                        height:(NSLayoutConstraint*)heightConstraint
                     topMargin:(NSLayoutConstraint*)topMarginConstraint
                     hintLabel:(UILabel*)hintLabel
            subviewConstraints:(NSArray*)constraints
                     forOffset:(CGFloat)offset
                   screenWidth:(CGFloat)screenWidth
                safeAreaInsets:(UIEdgeInsets)safeAreaInsets;

// Adds views necessary to customize the NTP search box.
- (void)addViewsToSearchField:(UIView*)searchField;

// TODO(crbug.com/807330) Remove post UI refresh.
// Animates legacy header view's |_shadow|'s alpha to 0.
- (void)fadeOutShadow;

// TODO(crbug.com/807330) Remove post UI refresh.
// Hides legacy toolbar subviews that should not be displayed on the new tab
// page.
- (void)hideToolbarViewsForNewTabPage;

// TODO(crbug.com/807330) Remove post UI refresh.
// Updates the toolbar tab count;
- (void)setToolbarTabCount:(int)tabCount;

// TODO(crbug.com/807330) Remove post UI refresh.
// |YES| if the toolbar can show the forward arrow.
- (void)setCanGoForward:(BOOL)canGoForward;

// TODO(crbug.com/807330) Remove post UI refresh.
// |YES| if the toolbar can show the back arrow.
- (void)setCanGoBack:(BOOL)canGoBack;

@optional
// Adds the |toolbarView| to the view implementing this protocol.
// Can only be added once.
- (void)addToolbarView:(UIView*)toolbarView;
// TODO(crbug.com/807330) Remove post UI refresh.
// Creates a NewTabPageToolbarController using the given |dispatcher|,
// |readingListModel|, and adds the toolbar view to self.
- (void)addToolbarWithReadingListModel:(ReadingListModel*)readingListModel
                            dispatcher:(id)dispatcher;
@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_
