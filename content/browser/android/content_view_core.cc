// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_view_core.h"

#include "content/browser/frame_host/interstitial_page_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_contents_view_android.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/user_agent.h"
#include "jni/ContentViewCoreImpl_jni.h"
#include "ui/android/window_android.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace content {

namespace {

RenderWidgetHostViewAndroid* GetRenderWidgetHostViewFromHost(
    RenderViewHost* host) {
  return static_cast<RenderWidgetHostViewAndroid*>(
      host->GetWidget()->GetView());
}

}  // namespace

ContentViewCore::ContentViewCore(JNIEnv* env,
                                 const JavaRef<jobject>& obj,
                                 WebContents* web_contents)
    : WebContentsObserver(web_contents),
      java_ref_(env, obj),
      web_contents_(static_cast<WebContentsImpl*>(web_contents)) {
  // Currently, the only use case we have for overriding a user agent involves
  // spoofing a desktop Linux user agent for "Request desktop site".
  // Automatically set it for all WebContents so that it is available when a
  // NavigationEntry requires the user agent to be overridden.
  const char kLinuxInfoStr[] = "X11; Linux x86_64";
  std::string product = content::GetContentClient()->GetProduct();
  std::string spoofed_ua =
      BuildUserAgentFromOSAndProduct(kLinuxInfoStr, product);
  web_contents->SetUserAgentOverride(spoofed_ua, false);
}

ContentViewCore::~ContentViewCore() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_obj = java_ref_.get(env);
  java_ref_.reset();
  if (!j_obj.is_null()) {
    Java_ContentViewCoreImpl_onNativeContentViewCoreDestroyed(
        env, j_obj, reinterpret_cast<intptr_t>(this));
  }
}

void ContentViewCore::OnJavaContentViewCoreDestroyed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK(env->IsSameObject(java_ref_.get(env).obj(), obj));
  java_ref_.reset();
  // Java peer has gone, ContentViewCore is not functional and waits to
  // be destroyed with WebContents.
  DCHECK(web_contents_);
}

void ContentViewCore::RenderViewReady() {
  WebContentsViewAndroid* view =
      static_cast<WebContentsViewAndroid*>(web_contents_->GetView());
  if (view->device_orientation() == 0)
    return;
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (rwhva)
    rwhva->UpdateScreenInfo(GetViewAndroid());

  web_contents_->OnScreenOrientationChange();
}

void ContentViewCore::WebContentsDestroyed() {
  delete this;
}

void ContentViewCore::RenderViewHostChanged(RenderViewHost* old_host,
                                            RenderViewHost* new_host) {
  if (old_host) {
    auto* view = GetRenderWidgetHostViewFromHost(old_host);
    if (view)
      view->UpdateNativeViewTree(nullptr);

    view = GetRenderWidgetHostViewFromHost(new_host);
    if (view)
      view->UpdateNativeViewTree(GetViewAndroid());
  }
  SetFocusInternal(GetViewAndroid()->HasFocus());
}

RenderWidgetHostViewAndroid* ContentViewCore::GetRenderWidgetHostViewAndroid()
    const {
  RenderWidgetHostView* rwhv = NULL;
  if (web_contents_) {
    rwhv = web_contents_->GetRenderWidgetHostView();
    if (web_contents_->ShowingInterstitialPage()) {
      rwhv = web_contents_->GetInterstitialPage()
                 ->GetMainFrame()
                 ->GetRenderViewHost()
                 ->GetWidget()
                 ->GetView();
    }
  }
  return static_cast<RenderWidgetHostViewAndroid*>(rwhv);
}

ui::ViewAndroid* ContentViewCore::GetViewAndroid() const {
  return web_contents_->GetView()->GetNativeView();
}

// ----------------------------------------------------------------------------
// Methods called from Java via JNI
// ----------------------------------------------------------------------------

void ContentViewCore::SetFocus(JNIEnv* env,
                               const JavaParamRef<jobject>& obj,
                               jboolean focused) {
  SetFocusInternal(focused);
}

void ContentViewCore::SetFocusInternal(bool focused) {
  if (!GetRenderWidgetHostViewAndroid())
    return;

  if (focused)
    GetRenderWidgetHostViewAndroid()->GotFocus();
  else
    GetRenderWidgetHostViewAndroid()->LostFocus();
}

// This is called for each ContentView.
jlong JNI_ContentViewCoreImpl_Init(JNIEnv* env,
                                   const JavaParamRef<jobject>& obj,
                                   const JavaParamRef<jobject>& jweb_contents) {
  WebContentsImpl* web_contents = static_cast<WebContentsImpl*>(
      WebContents::FromJavaWebContents(jweb_contents));
  CHECK(web_contents)
      << "A ContentViewCore should be created with a valid WebContents.";
  return reinterpret_cast<intptr_t>(
      new ContentViewCore(env, obj, web_contents));
}

}  // namespace content
