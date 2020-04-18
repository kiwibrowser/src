// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_ANDROID_H_
#define CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_ANDROID_H_

#include <jni.h>
#include <string>

namespace chrome {
namespace android {

std::string GetCaptivePortalServerUrl(JNIEnv* env);

void ReportNetworkConnectivity(JNIEnv* env);

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_ANDROID_H_
