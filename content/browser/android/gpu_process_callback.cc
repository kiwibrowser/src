// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/scoped_java_ref.h"
#include "base/android/unguessable_token_android.h"
#include "content/browser/android/scoped_surface_request_manager.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/ipc/common/gpu_surface_tracker.h"

#include "jni/GpuProcessCallback_jni.h"

namespace content {

void JNI_GpuProcessCallback_CompleteScopedSurfaceRequest(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz,
    const base::android::JavaParamRef<jobject>& token,
    const base::android::JavaParamRef<jobject>& surface) {
  base::UnguessableToken requestToken =
      base::android::UnguessableTokenAndroid::FromJavaUnguessableToken(env,
                                                                       token);
  if (!requestToken) {
    DLOG(ERROR) << "Received invalid surface request token.";
    return;
  }

  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::android::ScopedJavaGlobalRef<jobject> jsurface;
  jsurface.Reset(env, surface);
  ScopedSurfaceRequestManager::GetInstance()->FulfillScopedSurfaceRequest(
      requestToken, gl::ScopedJavaSurface(jsurface));
}

base::android::ScopedJavaLocalRef<jobject>
JNI_GpuProcessCallback_GetViewSurface(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    jint surface_id) {
  gl::ScopedJavaSurface surface_view =
      gpu::GpuSurfaceTracker::GetInstance()->AcquireJavaSurface(surface_id);
  return base::android::ScopedJavaLocalRef<jobject>(surface_view.j_surface());
}

}  // namespace content
