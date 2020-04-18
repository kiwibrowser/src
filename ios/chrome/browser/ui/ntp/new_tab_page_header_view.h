// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/ntp_header_view_adapter.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"

class ReadingListModel;

// Header view for the Material Design NTP. The header view contains all views
// that are displayed above the list of most visited sites, which includes the
// toolbar buttons, Google doodle, and fake omnibox.
@interface NewTabPageHeaderView : UIView<NTPHeaderViewAdapter>

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_VIEW_H_
