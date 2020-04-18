// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_TEST_UTIL_H_
#define IOS_CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_TEST_UTIL_H_

namespace tab_usage_recorder_test_util {

// Opens a new incognito tab using the UI and evicts any main tab model tabs.
void OpenNewIncognitoTabUsingUIAndEvictMainTabs();

// Switches to normal mode using the tab switcher and selects the
// previously-selected normal tab. Assumes current mode is Incognito.
void SwitchToNormalMode();

}  // namespace tab_usage_recorder_test_util

#endif  // IOS_CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_TEST_UTIL_H_
