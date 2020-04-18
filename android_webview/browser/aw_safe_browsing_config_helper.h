// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_CONFIG_HELPER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_CONFIG_HELPER_H_

#include "base/macros.h"

namespace android_webview {

class AwSafeBrowsingConfigHelper {
 public:
  static bool GetSafeBrowsingEnabledByManifest();
  static void SetSafeBrowsingEnabledByManifest(bool enabled);

 private:
  AwSafeBrowsingConfigHelper();
  DISALLOW_COPY_AND_ASSIGN(AwSafeBrowsingConfigHelper);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_CONFIG_HELPER_H_
