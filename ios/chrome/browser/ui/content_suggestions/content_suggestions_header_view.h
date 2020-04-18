// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_

#import "ios/chrome/browser/ui/ntp/ntp_header_view_adapter.h"

// Header view for the NTP. The header view contains all views that are
// displayed above the list of most visited sites, which includes the
// primary toolbar, doodle, and fake omnibox.
@interface ContentSuggestionsHeaderView : UIView<NTPHeaderViewAdapter>

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_
