// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/histogram_macros.h"
#include "jni/AppMenuDragHelper_jni.h"

using base::android::JavaParamRef;

// static
void JNI_AppMenuDragHelper_RecordAppMenuTouchDuration(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    jlong time_ms) {
  UMA_HISTOGRAM_TIMES("WrenchMenu.TouchDuration",
                      base::TimeDelta::FromMilliseconds(time_ms));
}
