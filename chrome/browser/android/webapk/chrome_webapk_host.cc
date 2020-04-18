// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/chrome_webapk_host.h"

#include "base/feature_list.h"
#include "chrome/browser/android/chrome_feature_list.h"

// static
bool ChromeWebApkHost::CanInstallWebApk() {
  return base::FeatureList::IsEnabled(chrome::android::kImprovedA2HS);
}
