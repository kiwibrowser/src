// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CONTENT_VIDEO_VIEW_H_
#define CONTENT_BROWSER_ANDROID_CONTENT_VIDEO_VIEW_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/gl/android/scoped_java_surface.h"

namespace gfx {
class Size;
}

namespace content {

class WebContents;

// Native mirror of ContentVideoView.java. This class is responsible for
// creating the Java video view and passing changes in player status to it.
// This class must be used on the UI thread.
class ContentVideoView {
 public:
  // Returns the singleton object or NULL.
  static ContentVideoView* GetInstance();

  class Client {
   public:
    Client() {}
    // For receiving notififcations when the SurfaceView surface is created and
    // destroyed. When |surface.IsEmpty()| the surface was destroyed and
    // the client should not hold any references to it once this returns.
    virtual void SetVideoSurface(gl::ScopedJavaSurface surface) = 0;

    // Called after the ContentVideoView has been hidden because we're exiting
    // fullscreen.
    virtual void DidExitFullscreen(bool release_media_player) = 0;

   protected:
    ~Client() {}

    DISALLOW_COPY_AND_ASSIGN(Client);
  };

  explicit ContentVideoView(Client* client,
                            WebContents* web_contents,
                            const base::android::JavaRef<jobject>& embedder,
                            const gfx::Size& video_natural_size);
  ~ContentVideoView();

  // To open another video on existing ContentVideoView.
  void OpenVideo();

  // Display an error dialog to the user.
  void OnMediaPlayerError(int error_type);

  // Update the video size.
  void OnVideoSizeChanged(int width, int height);

  // Exit fullscreen and notify |client_| with |DidExitFullscreen|.
  void ExitFullscreen();

  // Returns the corresponding ContentVideoView Java object if any.
  base::android::ScopedJavaLocalRef<jobject> GetJavaObject(JNIEnv* env);

  // Called by the Java class when the surface changes.
  void SetSurface(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  const base::android::JavaParamRef<jobject>& surface);

  // Called when the Java fullscreen view is destroyed. If
  // |release_media_player| is true, |client_| needs to release the player
  // as we are quitting the app.
  void DidExitFullscreen(JNIEnv*,
                         const base::android::JavaParamRef<jobject>&,
                         jboolean release_media_player);

  // Functions called to record fullscreen playback UMA metrics.
  void RecordFullscreenPlayback(JNIEnv*,
                                const base::android::JavaParamRef<jobject>&,
                                bool is_portrait_video,
                                bool is_orientation_portrait);
  void RecordExitFullscreenPlayback(
      JNIEnv*,
      const base::android::JavaParamRef<jobject>&,
      bool is_portrait_video,
      long playback_duration_in_milliseconds_before_orientation_change,
      long playback_duration_in_milliseconds_after_orientation_change);

 private:
  // Creates the corresponding ContentVideoView Java object.
  JavaObjectWeakGlobalRef CreateJavaObject(
      WebContents* web_contents,
      const base::android::JavaRef<jobject>& j_content_video_view_embedder,
      const gfx::Size& video_natural_size);

  Client* client_;

  // Weak reference to corresponding Java object.
  JavaObjectWeakGlobalRef j_content_video_view_;

  // Weak pointer for posting tasks.
  base::WeakPtrFactory<ContentVideoView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContentVideoView);
};

} // namespace content

#endif  // CONTENT_BROWSER_ANDROID_CONTENT_VIDEO_VIEW_H_
