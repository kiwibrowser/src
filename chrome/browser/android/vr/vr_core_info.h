// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_VR_CORE_INFO_H_
#define CHROME_BROWSER_ANDROID_VR_VR_CORE_INFO_H_

#include <jni.h>

#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"

namespace vr {

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.vr_shell
enum VrCoreCompatibility {
  VR_CORE_COMPATIBILITY_VR_NOT_SUPPORTED = 0,
  VR_CORE_COMPATIBILITY_VR_NOT_AVAILABLE = 1,
  VR_CORE_COMPATIBILITY_VR_OUT_OF_DATE = 2,
  VR_CORE_COMPATIBILITY_VR_READY = 3,

  VR_CORE_COMPATIBILITY_LAST = VR_CORE_COMPATIBILITY_VR_READY,
};

struct VrCoreInfo {
  const gvr_version gvr_sdk_version;
  const VrCoreCompatibility compatibility;

  VrCoreInfo(int32_t major_version,
             int32_t minor_version,
             int32_t patch_version,
             VrCoreCompatibility compatibility);
};

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_VR_CORE_INFO_H_
