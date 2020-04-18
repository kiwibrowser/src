// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "jni/SessionTabHelper_jni.h"

using base::android::JavaParamRef;

// static
jint JNI_SessionTabHelper_IdForTab(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jobject>& java_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);
  CHECK(web_contents);
  return SessionTabHelper::IdForTab(web_contents).id();
}
