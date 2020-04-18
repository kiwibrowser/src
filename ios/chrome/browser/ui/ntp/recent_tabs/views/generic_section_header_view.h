// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_GENERIC_SECTION_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_GENERIC_SECTION_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/header_of_collapsable_section_protocol.h"

namespace recent_tabs {

enum SectionHeaderType {
  RECENTLY_CLOSED_TABS_SECTION_HEADER,
  OTHER_DEVICES_SECTION_HEADER
};

}  // namespace recent_tabs

// View for the generic section header.
@interface GenericSectionHeaderView : UIView<HeaderOfCollapsableSectionProtocol>

// Designated initializer.
- (instancetype)initWithType:(recent_tabs::SectionHeaderType)type
          sectionIsCollapsed:(BOOL)collapsed;

// Returns the desired height when included in a UITableViewCell.
+ (CGFloat)desiredHeightInUITableViewCell;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_GENERIC_SECTION_HEADER_VIEW_H_
