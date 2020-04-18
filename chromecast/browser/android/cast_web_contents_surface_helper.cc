// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/android/cast_web_contents_surface_helper.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"
#include "jni/CastWebContentsSurfaceHelper_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace chromecast {
namespace shell {

namespace {
const void* kCastWebContentsSurfaceHelperData;
const void* kCastWebContentsSurfaceHelperKey =
    static_cast<const void*>(&kCastWebContentsSurfaceHelperData);
}  // namespace

// static
void JNI_CastWebContentsSurfaceHelper_SetContentVideoViewEmbedder(
    JNIEnv* env,
    const JavaParamRef<jobject>& j_caller,
    const JavaParamRef<jobject>& j_web_contents,
    const JavaParamRef<jobject>& j_embedder) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(j_web_contents);
  DCHECK(web_contents);
  CastWebContentsSurfaceHelper* fragment =
      CastWebContentsSurfaceHelper::Get(web_contents);
  fragment->SetContentVideoViewEmbedder(j_embedder);
}

// static
CastWebContentsSurfaceHelper* CastWebContentsSurfaceHelper::Get(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  CastWebContentsSurfaceHelper* instance =
      static_cast<CastWebContentsSurfaceHelper*>(
          web_contents->GetUserData(kCastWebContentsSurfaceHelperKey));
  if (!instance) {
    instance = new CastWebContentsSurfaceHelper();
    web_contents->SetUserData(kCastWebContentsSurfaceHelperKey,
                              base::WrapUnique(instance));
  }
  return instance;
}

CastWebContentsSurfaceHelper::CastWebContentsSurfaceHelper() {}

CastWebContentsSurfaceHelper::~CastWebContentsSurfaceHelper() {}

ScopedJavaLocalRef<jobject>
CastWebContentsSurfaceHelper ::GetContentVideoViewEmbedder() {
  return ScopedJavaLocalRef<jobject>(content_video_view_embedder_);
}

void CastWebContentsSurfaceHelper::SetContentVideoViewEmbedder(
    const JavaParamRef<jobject>& content_video_view_embedder) {
  content_video_view_embedder_ =
      ScopedJavaGlobalRef<jobject>(content_video_view_embedder);
}

}  // namespace shell
}  // namespace chromecast
