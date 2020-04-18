// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_IN_PROGRESS_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_IN_PROGRESS_VIEW_H_

#import <UIKit/UIKit.h>

// View indicating that the syncing is currently occurring.
@interface SignedInSyncInProgressView : UIView

// Designated initializer.
- (instancetype)initWithFrame:(CGRect)aRect;

// Returns the desired height when included in a UITableViewCell.
+ (CGFloat)desiredHeightInUITableViewCell;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SIGNED_IN_SYNC_IN_PROGRESS_VIEW_H_
