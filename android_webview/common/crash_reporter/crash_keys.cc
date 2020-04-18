// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/common/crash_reporter/crash_keys.h"

#include "components/crash/content/app/breakpad_linux.h"

namespace android_webview {
namespace crash_keys {

const char kAppPackageName[] = "app-package-name";
const char kAppPackageVersionCode[] = "app-package-version-code";

const char kAndroidSdkInt[] = "android-sdk-int";

// clang-format off
const char* const kWebViewCrashKeyWhiteList[] = {
    "AW_WHITELISTED_DEBUG_KEY",
    kAppPackageName,
    kAppPackageVersionCode,
    kAndroidSdkInt,

    // gpu
    "gpu-driver",
    "gpu-psver",
    "gpu-vsver",
    "gpu-gl-vendor",
    "gpu-gl-vendor__1",
    "gpu-gl-vendor__2",
    "gpu-gl-renderer",

    // content/:
    "bad_message_reason",
    "discardable-memory-allocated",
    "discardable-memory-free",
    "mojo-message-error__1",
    "mojo-message-error__2",
    "mojo-message-error__3",
    "mojo-message-error__4",
    "total-discardable-memory-allocated",
    nullptr};
// clang-format on

void InitCrashKeysForWebViewTesting() {
  breakpad::InitCrashKeysForTesting();
}

}  // namespace crash_keys

}  // namespace android_webview
