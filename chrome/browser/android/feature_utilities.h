// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_FEATURE_UTILITIES_H_
#define CHROME_BROWSER_ANDROID_FEATURE_UTILITIES_H_

#include <jni.h>

namespace chrome {
namespace android {

enum CustomTabsVisibilityHistogram {
  VISIBLE_CUSTOM_TAB,
  VISIBLE_CHROME_TAB,
  CUSTOM_TABS_VISIBILITY_MAX
};

CustomTabsVisibilityHistogram GetCustomTabsVisibleValue();

bool GetIsInMultiWindowModeValue();

bool GetIsChromeModernDesignEnabled();

} // namespace android
} // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_FEATURE_UTILITIES_H_
