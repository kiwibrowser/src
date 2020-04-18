// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_OFF_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_OFF_VIEW_H_

#import <UIKit/UIKit.h>

@protocol SyncPresenter;

namespace ios {
class ChromeBrowserState;
}

// View displaying a tab's title and favicon.
@interface SignedInSyncOffView : UIView

// Designated initializer.
- (instancetype)initWithFrame:(CGRect)aRect
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SyncPresenter>)presenter
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Returns the desired height when included in a UITableViewCell.
+ (CGFloat)desiredHeightInUITableViewCell;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_OFF_VIEW_H_
