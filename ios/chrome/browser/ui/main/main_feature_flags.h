// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_MAIN_FEATURE_FLAGS_H_
#define IOS_CHROME_BROWSER_UI_MAIN_MAIN_FEATURE_FLAGS_H_

#include "base/feature_list.h"

extern const base::Feature kTabSwitcherPresentsBVC;

// Returns whether the tab switcher will present the BVC..
bool TabSwitcherPresentsBVCEnabled();

#endif  // IOS_CHROME_BROWSER_UI_MAIN_MAIN_FEATURE_FLAGS_H_
