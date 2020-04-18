// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/android/remote/media_controller_bridge.h"

#include "jni/MediaControllerBridge_jni.h"

namespace media_router {

MediaControllerBridge::MediaControllerBridge(
    base::android::ScopedJavaGlobalRef<jobject> controller)
    : j_media_controller_bridge_(controller) {}

MediaControllerBridge::~MediaControllerBridge() = default;

void MediaControllerBridge::Play() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_MediaControllerBridge_play(env, j_media_controller_bridge_);
}

void MediaControllerBridge::Pause() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_MediaControllerBridge_pause(env, j_media_controller_bridge_);
}

void MediaControllerBridge::SetMute(bool mute) {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_MediaControllerBridge_setMute(env, j_media_controller_bridge_, mute);
}

void MediaControllerBridge::SetVolume(float volume) {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_MediaControllerBridge_setVolume(env, j_media_controller_bridge_, volume);
}

void MediaControllerBridge::Seek(base::TimeDelta time) {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_MediaControllerBridge_seek(env, j_media_controller_bridge_,
                                  time.InMilliseconds());
}

}  // namespace media_router
