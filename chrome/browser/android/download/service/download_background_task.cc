// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/callback_android.h"
#include "base/callback_forward.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/download/public/background_service/download_service.h"
#include "content/public/browser/browser_context.h"
#include "jni/DownloadBackgroundTask_jni.h"

using base::android::JavaParamRef;

namespace download {
namespace android {

void CallTaskFinishedCallback(const base::android::JavaRef<jobject>& j_callback,
                              bool needs_reschedule) {
  RunCallbackAndroid(j_callback, needs_reschedule);
}

// static
void JNI_DownloadBackgroundTask_StartBackgroundTask(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& jprofile,
    jint task_type,
    const base::android::JavaParamRef<jobject>& jcallback) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  TaskFinishedCallback finish_callback =
      base::Bind(&CallTaskFinishedCallback,
                 base::android::ScopedJavaGlobalRef<jobject>(jcallback));

  DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(profile);
  download_service->OnStartScheduledTask(
      static_cast<DownloadTaskType>(task_type), finish_callback);
}

// static
jboolean JNI_DownloadBackgroundTask_StopBackgroundTask(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& jprofile,
    jint task_type) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(profile);
  return download_service->OnStopScheduledTask(
      static_cast<DownloadTaskType>(task_type));
}

}  // namespace android
}  // namespace download
