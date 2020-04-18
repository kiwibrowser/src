// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_

@protocol TabHistoryPresentation
@optional
// UIView in which the Tab History popup will be presented.
- (UIView*)viewForTabHistoryPresentation;
// Tells the receiver the Tab History popup will be presented.
- (void)prepareForTabHistoryPresentation;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_PRESENTATION_H_
