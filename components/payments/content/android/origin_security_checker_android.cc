// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "components/payments/content/origin_security_checker.h"
#include "content/public/browser/web_contents.h"
#include "jni/OriginSecurityChecker_jni.h"

namespace payments {
namespace {

using ::base::android::JavaParamRef;
using ::base::android::ConvertJavaStringToUTF8;

}  // namespace

// static
jboolean JNI_OriginSecurityChecker_IsOriginSecure(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& jurl) {
  return OriginSecurityChecker::IsOriginSecure(
      GURL(ConvertJavaStringToUTF8(env, jurl)));
}

// static
jboolean JNI_OriginSecurityChecker_IsSchemeCryptographic(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& jurl) {
  return OriginSecurityChecker::IsSchemeCryptographic(
      GURL(ConvertJavaStringToUTF8(env, jurl)));
}

// static
jboolean JNI_OriginSecurityChecker_IsOriginLocalhostOrFile(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& jurl) {
  return OriginSecurityChecker::IsOriginLocalhostOrFile(
      GURL(ConvertJavaStringToUTF8(env, jurl)));
}

}  // namespace payments
