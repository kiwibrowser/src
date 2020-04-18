// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_UI_UPDATER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_UI_UPDATER_H_

#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_constants.h"

@protocol TabHistoryUIUpdater
// Tells the receiver to update its UI now that TabHistory popup will be
// presented.
- (void)updateUIForTabHistoryPresentationFrom:(ToolbarButtonType)button;

// Tells the receiver to update its UI now that TabHistory popup was dismissed.
- (void)updateUIForTabHistoryWasDismissed;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_REQUIREMENTS_TAB_HISTORY_UI_UPDATER_H_
