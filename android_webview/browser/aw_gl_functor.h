// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_GL_FUNCTOR_H_
#define ANDROID_WEBVIEW_BROWSER_AW_GL_FUNCTOR_H_

#include "android_webview/browser/compositor_frame_consumer.h"
#include "android_webview/browser/render_thread_manager.h"
#include "android_webview/browser/render_thread_manager_client.h"
#include "base/android/jni_weak_ref.h"

namespace android_webview {

class AwGLFunctor : public RenderThreadManagerClient {
 public:
  bool RequestInvokeGL(bool wait_for_completion) override;
  void DetachFunctorFromView() override;

  AwGLFunctor(const JavaObjectWeakGlobalRef& java_ref);
  ~AwGLFunctor() override;

  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void DeleteHardwareRenderer(JNIEnv* env,
                              const base::android::JavaParamRef<jobject>& obj);
  jlong GetAwDrawGLViewContext(JNIEnv* env,
                               const base::android::JavaParamRef<jobject>& obj);
  jlong GetAwDrawGLFunction(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

  CompositorFrameConsumer* GetCompositorFrameConsumer() {
    return &render_thread_manager_;
  }

 private:
  JavaObjectWeakGlobalRef java_ref_;
  RenderThreadManager render_thread_manager_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_GL_FUNCTOR_H_
