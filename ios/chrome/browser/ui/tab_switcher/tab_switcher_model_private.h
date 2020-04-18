// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_PRIVATE_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_PRIVATE_H_

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"

@interface TabSwitcherModel (TestingSupport)
// Notifies |delegate| about changes from |oldSessions| to |newSessions|,
// including the insertion/deletions of distant sessions, and the
// insertions/deletions/updates of tabs in the distant sessions.
// Does not notify the delegate when the ordering of the distant sessions
// changes.
+ (void)notifyDelegate:(id<TabSwitcherModelDelegate>)delegate
       aboutChangeFrom:(synced_sessions::SyncedSessions&)oldSessions
                    to:(synced_sessions::SyncedSessions&)newSessions;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_PRIVATE_H_
