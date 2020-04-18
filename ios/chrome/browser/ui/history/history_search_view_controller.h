// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@class HistorySearchViewController;

// Delegate for HistorySearchViewController. Used to pass on search bar
// interactions.
@protocol HistorySearchViewControllerDelegate<NSObject>
// Called when the search button is clicked.
- (void)historySearchViewController:
            (HistorySearchViewController*)historySearchViewController
            didRequestSearchForTerm:(NSString*)searchTerm;
// Called when the search view's cancel button is clicked.
- (void)historySearchViewControllerDidCancel:
    (HistorySearchViewController*)historySearchViewController;
@end

// Controller for the search bar used in the history panel.
@interface HistorySearchViewController : UIViewController

// Delegate for forwarding interactions with the search view.
@property(nonatomic, weak) id<HistorySearchViewControllerDelegate> delegate;
// YES if the search view is enabled. Setting this property enables or disables
// the search view, as appropriate.
@property(nonatomic, getter=isEnabled) BOOL enabled;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SEARCH_VIEW_CONTROLLER_H_
