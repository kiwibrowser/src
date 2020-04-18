// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_video_view.h"

#include "base/metrics/histogram_macros.h"
#include "content/public/browser/web_contents.h"
#include "jni/ContentVideoViewImpl_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace content {

namespace {
// There can only be one content video view at a time, this holds onto that
// singleton instance.
ContentVideoView* g_content_video_view = NULL;

}  // namespace

static ScopedJavaLocalRef<jobject>
JNI_ContentVideoViewImpl_GetSingletonJavaContentVideoView(
    JNIEnv* env,
    const JavaParamRef<jclass>&) {
  if (g_content_video_view)
    return g_content_video_view->GetJavaObject(env);
  else
    return ScopedJavaLocalRef<jobject>();
}

ContentVideoView* ContentVideoView::GetInstance() {
  return g_content_video_view;
}

ContentVideoView::ContentVideoView(Client* client,
                                   WebContents* web_contents,
                                   const JavaRef<jobject>& video_embedder,
                                   const gfx::Size& video_natural_size)
    : client_(client), weak_factory_(this) {
  DCHECK(!g_content_video_view);
  j_content_video_view_ =
      CreateJavaObject(web_contents, video_embedder, video_natural_size);
  g_content_video_view = this;
}

ContentVideoView::~ContentVideoView() {
  DCHECK(g_content_video_view);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> content_video_view = GetJavaObject(env);
  if (!content_video_view.is_null()) {
    Java_ContentVideoViewImpl_destroyContentVideoView(env, content_video_view,
                                                      true);
    j_content_video_view_.reset();
  }
  g_content_video_view = NULL;
}

void ContentVideoView::OpenVideo() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> content_video_view = GetJavaObject(env);
  if (!content_video_view.is_null()) {
    Java_ContentVideoViewImpl_openVideo(env, content_video_view);
  }
}

void ContentVideoView::OnMediaPlayerError(int error_type) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> content_video_view = GetJavaObject(env);
  if (!content_video_view.is_null()) {
    Java_ContentVideoViewImpl_onMediaPlayerError(env, content_video_view,
                                                 error_type);
  }
}

void ContentVideoView::OnVideoSizeChanged(int width, int height) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> content_video_view = GetJavaObject(env);
  if (!content_video_view.is_null()) {
    Java_ContentVideoViewImpl_onVideoSizeChanged(env, content_video_view, width,
                                                 height);
  }
}

void ContentVideoView::ExitFullscreen() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> content_video_view = GetJavaObject(env);
  bool release_media_player = false;
  if (!content_video_view.is_null())
    Java_ContentVideoViewImpl_exitFullscreen(env, content_video_view,
                                             release_media_player);
}

ScopedJavaLocalRef<jobject> ContentVideoView::GetJavaObject(JNIEnv* env) {
  return j_content_video_view_.get(env);
}

void ContentVideoView::SetSurface(JNIEnv*,
                                  const JavaParamRef<jobject>&,
                                  const JavaParamRef<jobject>& surface) {
  client_->SetVideoSurface(
      gl::ScopedJavaSurface::AcquireExternalSurface(surface));
}

void ContentVideoView::DidExitFullscreen(JNIEnv*,
                                         const JavaParamRef<jobject>&,
                                         jboolean release_media_player) {
  j_content_video_view_.reset();
  client_->DidExitFullscreen(release_media_player);
}

void ContentVideoView::RecordFullscreenPlayback(JNIEnv*,
                                                const JavaParamRef<jobject>&,
                                                bool is_portrait_video,
                                                bool is_orientation_portrait) {
  UMA_HISTOGRAM_BOOLEAN("MobileFullscreenVideo.OrientationPortrait",
                        is_orientation_portrait);
  UMA_HISTOGRAM_BOOLEAN("MobileFullscreenVideo.VideoPortrait",
                        is_portrait_video);
}

void ContentVideoView::RecordExitFullscreenPlayback(
    JNIEnv*,
    const JavaParamRef<jobject>&,
    bool is_portrait_video,
    long playback_duration_in_milliseconds_before_orientation_change,
    long playback_duration_in_milliseconds_after_orientation_change) {
  bool orientation_changed = (
      playback_duration_in_milliseconds_after_orientation_change != 0);
  if (is_portrait_video) {
    UMA_HISTOGRAM_COUNTS(
        "MobileFullscreenVideo.PortraitDuration",
        playback_duration_in_milliseconds_before_orientation_change);
    UMA_HISTOGRAM_COUNTS(
        "MobileFullscreenVideo.PortraitRotation", orientation_changed);
    if (orientation_changed) {
      UMA_HISTOGRAM_COUNTS(
          "MobileFullscreenVideo.DurationAfterPotraitRotation",
          playback_duration_in_milliseconds_after_orientation_change);
    }
  } else {
    UMA_HISTOGRAM_COUNTS(
        "MobileFullscreenVideo.LandscapeDuration",
        playback_duration_in_milliseconds_before_orientation_change);
    UMA_HISTOGRAM_COUNTS(
        "MobileFullscreenVideo.LandscapeRotation", orientation_changed);
  }
}

JavaObjectWeakGlobalRef ContentVideoView::CreateJavaObject(
    WebContents* web_contents,
    const JavaRef<jobject>& j_content_video_view_embedder,
    const gfx::Size& video_natural_size) {
  JNIEnv* env = AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> j_web_contents =
      web_contents->GetJavaWebContents();

  if (j_web_contents.is_null())
    return JavaObjectWeakGlobalRef(env, nullptr);

  return JavaObjectWeakGlobalRef(
      env, Java_ContentVideoViewImpl_createContentVideoView(
               env, j_web_contents, j_content_video_view_embedder,
               reinterpret_cast<intptr_t>(this), video_natural_size.width(),
               video_natural_size.height())
               .obj());
}
}  // namespace content
