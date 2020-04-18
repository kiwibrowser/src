// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "jni/UseZoomForDSFPolicy_jni.h"

using base::android::JavaParamRef;

namespace content {

jboolean JNI_UseZoomForDSFPolicy_IsUseZoomForDSFEnabled(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return IsUseZoomForDSFEnabled();
}

}  // namespace content
