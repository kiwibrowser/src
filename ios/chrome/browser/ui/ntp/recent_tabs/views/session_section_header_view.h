// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_SECTION_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_SECTION_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/header_of_collapsable_section_protocol.h"

namespace synced_sessions {
class DistantSession;
}  // namespace synced_sessions

// View for the header of the sessions section.
@interface SessionSectionHeaderView : UIView<HeaderOfCollapsableSectionProtocol>

// Designated initializer.
- (instancetype)initWithFrame:(CGRect)aRect sectionIsCollapsed:(BOOL)collapsed;

// Updates view to display information for |distantSession|.
- (void)updateWithSession:
    (synced_sessions::DistantSession const*)distantSession;

// Returns the desired height when included in a UITableViewCell.
+ (CGFloat)desiredHeightInUITableViewCell;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_SESSION_SECTION_HEADER_VIEW_H_
