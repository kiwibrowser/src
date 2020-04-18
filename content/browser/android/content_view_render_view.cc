// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_view_render_view.h"

#include <android/bitmap.h>
#include <android/native_window_jni.h>

#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "cc/layers/layer.h"
#include "content/public/browser/android/compositor.h"
#include "content/public/browser/android/content_view_layer_renderer.h"
#include "content/public/browser/web_contents.h"
#include "jni/ContentViewRenderView_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/size.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

ContentViewRenderView::ContentViewRenderView(JNIEnv* env,
                                             jobject obj,
                                             gfx::NativeWindow root_window)
    : root_window_(root_window), current_surface_format_(0) {
  java_obj_.Reset(env, obj);
}

ContentViewRenderView::~ContentViewRenderView() {
}

// static
static jlong JNI_ContentViewRenderView_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& jroot_window_android) {
  gfx::NativeWindow root_window =
      ui::WindowAndroid::FromJavaWindowAndroid(jroot_window_android);
  ContentViewRenderView* content_view_render_view =
      new ContentViewRenderView(env, obj, root_window);
  return reinterpret_cast<intptr_t>(content_view_render_view);
}

void ContentViewRenderView::Destroy(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  delete this;
}

void ContentViewRenderView::SetCurrentWebContents(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& jweb_contents) {
  InitCompositor();
  WebContents* web_contents = WebContents::FromJavaWebContents(jweb_contents);
  compositor_->SetRootLayer(web_contents
                                ? web_contents->GetNativeView()->GetLayer()
                                : scoped_refptr<cc::Layer>());
}

void ContentViewRenderView::OnPhysicalBackingSizeChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& jweb_contents,
    jint width,
    jint height) {
  WebContents* web_contents = WebContents::FromJavaWebContents(jweb_contents);
  gfx::Size size(width, height);
  web_contents->GetNativeView()->OnPhysicalBackingSizeChanged(size);
}

void ContentViewRenderView::SurfaceCreated(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj) {
  current_surface_format_ = 0;
  InitCompositor();
}

void ContentViewRenderView::SurfaceDestroyed(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  compositor_->SetSurface(NULL);
  current_surface_format_ = 0;
}

void ContentViewRenderView::SurfaceChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint format,
    jint width,
    jint height,
    const JavaParamRef<jobject>& surface) {
  if (current_surface_format_ != format) {
    current_surface_format_ = format;
    compositor_->SetSurface(surface);
  }
  compositor_->SetWindowBounds(gfx::Size(width, height));
}

void ContentViewRenderView::SetOverlayVideoMode(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    bool enabled) {
  compositor_->SetRequiresAlphaChannel(enabled);
  compositor_->SetBackgroundColor(enabled ? SK_ColorTRANSPARENT
                                          : SK_ColorWHITE);
  compositor_->SetNeedsComposite();
}

void ContentViewRenderView::SetNeedsComposite(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  compositor_->SetNeedsComposite();
}

void ContentViewRenderView::UpdateLayerTreeHost() {
  // TODO(wkorman): Rename Layout to UpdateLayerTreeHost in all Android
  // Compositor related classes.
}

void ContentViewRenderView::DidSwapFrame(int pending_frames) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ContentViewRenderView_didSwapFrame(env, java_obj_);
}

void ContentViewRenderView::InitCompositor() {
  if (!compositor_)
    compositor_.reset(Compositor::Create(this, root_window_));
}

}  // namespace content
