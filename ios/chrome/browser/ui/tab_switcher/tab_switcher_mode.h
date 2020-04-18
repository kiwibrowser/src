// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODE_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODE_H_

// The style of tab switcher.
enum class TabSwitcherMode { STACK, TABLET_SWITCHER, GRID };

// Returns the current tab switcher mode.
TabSwitcherMode GetTabSwitcherMode();

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODE_H_
