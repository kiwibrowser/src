// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_CONTROLLER_PROTOCOL_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_CONTROLLER_PROTOCOL_H_

// Delete for NTP and it's subclasses to communicate with the toolbar.
@protocol NewTabPageControllerDelegate
// Sets the background color of the toolbar to the color of the incognito NTP,
// with an |alpha|.
- (void)setToolbarBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha;
// Sets the toolbar location bar alpha and vertical offset based on |progress|.
- (void)setScrollProgressForTabletOmnibox:(CGFloat)progress;
@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_CONTROLLER_PROTOCOL_H_
