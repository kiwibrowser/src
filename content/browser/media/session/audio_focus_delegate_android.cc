// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/audio_focus_delegate_android.h"

#include "base/android/jni_android.h"
#include "content/browser/media/session/media_session_impl.h"
#include "jni/AudioFocusDelegate_jni.h"

using base::android::JavaParamRef;

namespace content {

AudioFocusDelegateAndroid::AudioFocusDelegateAndroid(
    MediaSessionImpl* media_session)
    : media_session_(media_session) {}

AudioFocusDelegateAndroid::~AudioFocusDelegateAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  Java_AudioFocusDelegate_tearDown(env, j_media_session_delegate_);
}

void AudioFocusDelegateAndroid::Initialize() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  j_media_session_delegate_.Reset(
      Java_AudioFocusDelegate_create(env, reinterpret_cast<intptr_t>(this)));
}

bool AudioFocusDelegateAndroid::RequestAudioFocus(
    AudioFocusManager::AudioFocusType audio_focus_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  return Java_AudioFocusDelegate_requestAudioFocus(
      env, j_media_session_delegate_,
      audio_focus_type ==
          AudioFocusManager::AudioFocusType::GainTransientMayDuck);
}

void AudioFocusDelegateAndroid::AbandonAudioFocus() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  Java_AudioFocusDelegate_abandonAudioFocus(env, j_media_session_delegate_);
}

void AudioFocusDelegateAndroid::OnSuspend(JNIEnv*,
                                          const JavaParamRef<jobject>&) {
  if (!media_session_->IsActive())
    return;

  media_session_->Suspend(MediaSession::SuspendType::SYSTEM);
}

void AudioFocusDelegateAndroid::OnResume(JNIEnv*,
                                         const JavaParamRef<jobject>&) {
  if (!media_session_->IsSuspended())
    return;

  media_session_->Resume(MediaSession::SuspendType::SYSTEM);
}

void AudioFocusDelegateAndroid::OnStartDucking(JNIEnv*, jobject) {
  media_session_->StartDucking();
}

void AudioFocusDelegateAndroid::OnStopDucking(JNIEnv*, jobject) {
  media_session_->StopDucking();
}

void AudioFocusDelegateAndroid::RecordSessionDuck(
    JNIEnv*,
    const JavaParamRef<jobject>&) {
  media_session_->RecordSessionDuck();
}

// static
std::unique_ptr<AudioFocusDelegate> AudioFocusDelegate::Create(
    MediaSessionImpl* media_session) {
  AudioFocusDelegateAndroid* delegate =
      new AudioFocusDelegateAndroid(media_session);
  delegate->Initialize();
  return std::unique_ptr<AudioFocusDelegate>(delegate);
}

}  // namespace content
