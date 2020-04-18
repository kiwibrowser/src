// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_H_

#import <UIKit/UIKit.h>

// View for displaying a search bar in history.
@interface HistorySearchView : UIView

// YES if the search view is in an enabled state. When not enabled, the search
// view text is greyed out, the clear button is removed, and the cancel button
// is disabled.
@property(nonatomic, getter=isEnabled) BOOL enabled;

// Sets the target/action of the cancel button.
- (void)setCancelTarget:(id)target action:(SEL)action;

// Sets the delegate of the search bar.
- (void)setSearchBarDelegate:(id<UITextFieldDelegate>)delegate;

// Clears the search bar text.
- (void)clearText;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_H_
