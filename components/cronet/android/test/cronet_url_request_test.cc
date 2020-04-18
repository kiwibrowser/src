// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jni/CronetUrlRequestTest_jni.h"
#include "net/base/load_flags.h"

using base::android::JavaParamRef;

static jint JNI_CronetUrlRequestTest_GetConnectionMigrationDisableLoadFlag(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller) {
  return net::LOAD_DISABLE_CONNECTION_MIGRATION;
}