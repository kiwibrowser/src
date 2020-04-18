// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_EGTEST_UTIL_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_EGTEST_UTIL_H_

@protocol GREYMatcher;

namespace chrome_test_util {

// Returns the GREYMatcher for the button that opens the tab switcher.
id<GREYMatcher> TabletTabSwitcherOpenButton();

// Returns the GREYMatcher for the button that closes the tab switcher.
id<GREYMatcher> TabletTabSwitcherCloseButton();

// Returns the GREYMatcher for the button that creates new non incognito tabs
// from within the tab switcher.
id<GREYMatcher> TabletTabSwitcherNewTabButton();

// Returns the GREYMatcher for the button that creates new incognito tabs from
// within the tab switcher.
id<GREYMatcher> TabletTabSwitcherNewIncognitoTabButton();

// Returns the GREYMatcher for the button that closes tabs on iPad.
id<GREYMatcher> TabletTabSwitcherCloseTabButton();

// Returns the GREYMatcher for the button to go to the non incognito panel in
// the tab switcher.
id<GREYMatcher> TabletTabSwitcherOpenTabsPanelButton();

// Returns the GREYMatcher for the button to go to the incognito panel in
// the tab switcher.
id<GREYMatcher> TabletTabSwitcherIncognitoTabsPanelButton();

// Returns the GREYMatcher for the button to go to the other devices panel in
// the tab switcher.
id<GREYMatcher> TabletTabSwitcherOtherDevicesPanelButton();

}  // namespace chrome_test_util

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_EGTEST_UTIL_H_
