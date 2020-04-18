// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/document/document_web_contents_delegate.h"

#include "components/web_contents_delegate_android/web_contents_delegate_android.h"
#include "content/public/browser/web_contents.h"
#include "jni/DocumentWebContentsDelegate_jni.h"

using base::android::JavaParamRef;

DocumentWebContentsDelegate::DocumentWebContentsDelegate(JNIEnv* env,
                                                         jobject obj)
    : WebContentsDelegateAndroid(env, obj) {
}

DocumentWebContentsDelegate::~DocumentWebContentsDelegate() {
}

void DocumentWebContentsDelegate::AttachContents(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcaller,
    const JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  web_contents->SetDelegate(this);
}

void DocumentWebContentsDelegate::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_pos,
    bool user_gesture,
    bool* was_blocked) {
  NOTREACHED();
}

void DocumentWebContentsDelegate::CloseContents(content::WebContents* source) {
  NOTREACHED();
}

bool DocumentWebContentsDelegate::ShouldCreateWebContents(
    content::WebContents* web_contents,
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  NOTREACHED();
  return false;
}

static jlong JNI_DocumentWebContentsDelegate_Initialize(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return reinterpret_cast<intptr_t>(new DocumentWebContentsDelegate(env, obj));
}
