// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_JAVA_UTILS_H_
#define CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_JAVA_UTILS_H_

#include <jni.h>
#include "base/android/scoped_java_ref.h"

namespace vr {

class ArCoreJavaUtils {
 public:
  static base::android::ScopedJavaLocalRef<jobject> GetApplicationContext();

  static bool EnsureLoaded();
};

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_JAVA_UTILS_H_
