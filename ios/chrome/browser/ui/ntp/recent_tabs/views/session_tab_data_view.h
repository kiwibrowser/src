// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_TAB_DATA_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_TAB_DATA_VIEW_H_

#import <UIKit/UIKit.h>

#include "components/sessions/core/tab_restore_service.h"

namespace ios {
class ChromeBrowserState;
}

namespace synced_sessions {
struct DistantTab;
}

class GURL;

// View displaying a tab's title and favicon.
@interface SessionTabDataView : UIView

// Designated initializer.
- (instancetype)initWithFrame:(CGRect)aRect;

// Updates view to display information for the |distantTab| using |browserState|
// to obtain favicons. |browserState| and |distantTab| must not be nil.
- (void)updateWithDistantTab:(synced_sessions::DistantTab const*)distantTab
                browserState:(ios::ChromeBrowserState*)browserState;

// Updates view to display information for the |entry| using |browserState| to
// obtain favicons. |browserState| and |entry| must not be nil.
- (void)updateWithTabRestoreEntry:
            (const sessions::TabRestoreService::Entry*)entry
                     browserState:(ios::ChromeBrowserState*)browserState;

// Updates view to display the |text| and |url| using |browserState| to obtain
// favicons. |text| and |browserState| must not be nil.
- (void)setText:(NSString*)text
             url:(const GURL&)url
    browserState:(ios::ChromeBrowserState*)browserState;

// Returns the desired height when included in a UITableViewCell.
+ (CGFloat)desiredHeightInUITableViewCell;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_TAB_DATA_VIEW_H_
