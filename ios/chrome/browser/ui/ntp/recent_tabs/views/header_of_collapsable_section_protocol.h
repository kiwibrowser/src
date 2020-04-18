// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_HEADER_OF_COLLAPSABLE_SECTION_PROTOCOL_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_HEADER_OF_COLLAPSABLE_SECTION_PROTOCOL_H_

// Implemented by views that are headers of collapsable sections.
@protocol HeaderOfCollapsableSectionProtocol

// Sets whether the section is collapsed with or without animation.
- (void)setSectionIsCollapsed:(BOOL)collapsed animated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_VIEWS_HEADER_OF_COLLAPSABLE_SECTION_PROTOCOL_H_
