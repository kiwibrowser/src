// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_EGTEST_UTIL_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_EGTEST_UTIL_H_

@protocol GREYMatcher;

namespace chrome_test_util {

// Returns the GREYMatcher for the button that opens the tab grid.
id<GREYMatcher> TabGridOpenButton();

// Returns the GREYMatcher for the button that closes the tab grid.
id<GREYMatcher> TabGridDoneButton();

// Returns the GREYMatcher for the button that closes all the tabs in the tab
// grid.
id<GREYMatcher> TabGridCloseAllButton();

// Returns the GREYMatcher for the button that reverts the close all tabs action
// in the tab grid.
id<GREYMatcher> TabGridUndoCloseAllButton();

// Returns the GREYMatcher for the regular tabs empty state view.
id<GREYMatcher> TabGridRegularTabsEmptyStateView();

// Returns the GREYMatcher for the button that creates new non incognito tabs
// from within the tab grid.
id<GREYMatcher> TabGridNewTabButton();

// Returns the GREYMatcher for the button that creates new incognito tabs from
// within the tab grid.
id<GREYMatcher> TabGridNewIncognitoTabButton();

// Returns the GREYMatcher for the button to go to the non incognito panel in
// the tab grid.
id<GREYMatcher> TabGridOpenTabsPanelButton();

// Returns the GREYMatcher for the button to go to the incognito panel in
// the tab grid.
id<GREYMatcher> TabGridIncognitoTabsPanelButton();

// Returns the GREYMatcher for the button to go to the other devices panel in
// the tab grid.
id<GREYMatcher> TabGridOtherDevicesPanelButton();

// Returns the GREYMatcher for the cell at |index| in the tab grid.
id<GREYMatcher> TabGridCellAtIndex(unsigned int index);

// Returns the GREYMatcher for the button to close the cell at |index| in the
// tab grid.
id<GREYMatcher> TabGridCloseButtonForCellAtIndex(unsigned int index);

}  // namespace chrome_test_util

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_EGTEST_UTIL_H_
