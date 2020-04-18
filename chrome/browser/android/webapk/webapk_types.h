// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_TYPES_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_TYPES_H_

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.webapps
//
// Indicates the reason that a WebAPK update is requested.
enum class WebApkUpdateReason {
  NONE,
  OLD_SHELL_APK,
  PRIMARY_ICON_HASH_DIFFERS,
  BADGE_ICON_HASH_DIFFERS,
  SCOPE_DIFFERS,
  START_URL_DIFFERS,
  SHORT_NAME_DIFFERS,
  NAME_DIFFERS,
  BACKGROUND_COLOR_DIFFERS,
  THEME_COLOR_DIFFERS,
  ORIENTATION_DIFFERS,
  DISPLAY_MODE_DIFFERS,
};

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_TYPES_H_
