// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/tap_disambiguator.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/public/browser/web_contents.h"
#include "jni/TapDisambiguator_jni.h"
#include "ui/gfx/android/java_bitmap.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

namespace {

ScopedJavaLocalRef<jobject> JNI_TapDisambiguator_CreateJavaRect(
    JNIEnv* env,
    const gfx::Rect& rect) {
  return ScopedJavaLocalRef<jobject>(Java_TapDisambiguator_createRect(
      env, rect.x(), rect.y(), rect.right(), rect.bottom()));
}

}  // namespace

jlong JNI_TapDisambiguator_Init(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jobject>& jweb_contents) {
  WebContents* web_contents = WebContents::FromJavaWebContents(jweb_contents);
  DCHECK(web_contents);

  // Owns itself and gets destroyed when |WebContentsDestroyed| is called.
  auto* tap_disambiguator = new TapDisambiguator(env, obj, web_contents);
  tap_disambiguator->Initialize();
  return reinterpret_cast<intptr_t>(tap_disambiguator);
}

TapDisambiguator::TapDisambiguator(JNIEnv* env,
                                   const JavaParamRef<jobject>& obj,
                                   WebContents* web_contents)
    : RenderWidgetHostConnector(web_contents), rwhva_(nullptr) {
  java_obj_ = JavaObjectWeakGlobalRef(env, obj);
}

TapDisambiguator::~TapDisambiguator() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_obj_.get(env);
  if (!obj.is_null())
    Java_TapDisambiguator_destroy(env, obj);
}

void TapDisambiguator::UpdateRenderProcessConnection(
    RenderWidgetHostViewAndroid* old_rwhva,
    RenderWidgetHostViewAndroid* new_rwhva) {
  if (old_rwhva)
    old_rwhva->set_tap_disambiguator(nullptr);
  if (new_rwhva)
    new_rwhva->set_tap_disambiguator(this);
  rwhva_ = new_rwhva;
}

void TapDisambiguator::ShowPopup(const gfx::Rect& rect_pixels,
                                 const SkBitmap& zoomed_bitmap) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_obj_.get(env);
  if (obj.is_null())
    return;

  ScopedJavaLocalRef<jobject> rect_object(
      JNI_TapDisambiguator_CreateJavaRect(env, rect_pixels));

  ScopedJavaLocalRef<jobject> java_bitmap =
      gfx::ConvertToJavaBitmap(&zoomed_bitmap);
  DCHECK(!java_bitmap.is_null());

  Java_TapDisambiguator_showPopup(env, obj, rect_object, java_bitmap);
}

void TapDisambiguator::HidePopup() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_obj_.get(env);
  if (obj.is_null())
    return;
  Java_TapDisambiguator_hidePopup(env, obj);
}

void TapDisambiguator::ResolveTapDisambiguation(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong time_ms,
    jfloat x,
    jfloat y,
    jboolean is_long_press) {
  if (!rwhva_)
    return;

  float dip_scale = rwhva_->GetNativeView()->GetDipScale();
  rwhva_->ResolveTapDisambiguation(
      time_ms / 1000, gfx::Point(x / dip_scale, y / dip_scale), is_long_press);
}

}  // namespace content
