// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_ANDROID_H_

#include <jni.h>
#include <memory>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/media_session_observer.h"

namespace content {

class MediaSessionImpl;
class WebContentsAndroid;

// This class is interlayer between native MediaSession and Java
// MediaSession. This class is owned by the native MediaSession and will
// teardown Java MediaSession when the native MediaSession is destroyed.
// Java MediaSessionObservers are also proxied via this class.
class MediaSessionAndroid final : public MediaSessionObserver {
 public:
  // Helper class for calling GetJavaObject() in a static method, in order to
  // avoid leaking the Java object outside.
  struct JavaObjectGetter;

  explicit MediaSessionAndroid(MediaSessionImpl* session);
  ~MediaSessionAndroid() override;

  // MediaSessionObserver implementation.
  void MediaSessionDestroyed() override;
  void MediaSessionStateChanged(bool is_controllable,
                                bool is_suspended) override;
  void MediaSessionMetadataChanged(
      const base::Optional<MediaMetadata>& metadata) override;
  void MediaSessionActionsChanged(
      const std::set<blink::mojom::MediaSessionAction>& actions) override;

  // MediaSession method wrappers.
  void Resume(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_obj);
  void Suspend(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_obj);
  void Stop(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_obj);
  void SeekForward(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& j_obj,
                   const jlong millis);
  void SeekBackward(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& j_obj,
                    const jlong millis);
  void DidReceiveAction(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& j_obj,
                        jint action);
  void RequestSystemAudioFocus(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& j_obj);

 private:
  WebContentsAndroid* GetWebContentsAndroid();

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  // The linked Java object. The strong reference is hold by Java WebContensImpl
  // to avoid introducing a new GC root.
  JavaObjectWeakGlobalRef j_media_session_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_ANDROID_H_
