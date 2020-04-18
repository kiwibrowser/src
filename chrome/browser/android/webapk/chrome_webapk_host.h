// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_CHROME_WEBAPK_HOST_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_CHROME_WEBAPK_HOST_H_

#include "base/macros.h"

// ChromeWebApkHost is the C++ counterpart of org.chromium.chrome.browser's
// ChromeWebApkHost in Java.
class ChromeWebApkHost {
 public:
  // Returns whether installing WebApk is possible.
  static bool CanInstallWebApk();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ChromeWebApkHost);
};

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_CHROME_WEBAPK_HOST_H_
