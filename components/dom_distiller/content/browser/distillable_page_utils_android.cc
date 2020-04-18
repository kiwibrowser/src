// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "content/public/browser/web_contents.h"
#include "jni/DistillablePageUtils_jni.h"

using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;

namespace dom_distiller {
namespace android {
namespace {
void OnIsPageDistillableUpdate(const JavaRef<jobject>& callback,
                               bool isDistillable,
                               bool isLast,
                               bool isMobileFriendly) {
  Java_DistillablePageUtils_callOnIsPageDistillableUpdate(
      base::android::AttachCurrentThread(), callback, isDistillable, isLast,
      isMobileFriendly);
}
}  // namespace

static void JNI_DistillablePageUtils_SetDelegate(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobject>& webContents,
    const JavaParamRef<jobject>& callback) {
  content::WebContents* web_contents(
      content::WebContents::FromJavaWebContents(webContents));
  if (!web_contents) {
    return;
  }

  DistillabilityDelegate delegate = base::Bind(
      OnIsPageDistillableUpdate, ScopedJavaGlobalRef<jobject>(env, callback));
  setDelegate(web_contents, delegate);
}

}
}
