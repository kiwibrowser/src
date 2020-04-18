// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/audio_focus_delegate.h"

#include "base/command_line.h"
#include "content/browser/media/session/audio_focus_manager.h"
#include "media/base/media_switches.h"

namespace content {

using AudioFocusType = AudioFocusManager::AudioFocusType;

namespace {

// AudioFocusDelegateDefault is the default implementation of
// AudioFocusDelegate which only handles audio focus between WebContents.
class AudioFocusDelegateDefault : public AudioFocusDelegate {
 public:
  explicit AudioFocusDelegateDefault(MediaSessionImpl* media_session);
  ~AudioFocusDelegateDefault() override;

  // AudioFocusDelegate implementation.
  bool RequestAudioFocus(
      AudioFocusManager::AudioFocusType audio_focus_type) override;
  void AbandonAudioFocus() override;

 private:
  // Weak pointer because |this| is owned by |media_session_|.
  MediaSessionImpl* media_session_;
};

}  // anonymous namespace

AudioFocusDelegateDefault::AudioFocusDelegateDefault(
    MediaSessionImpl* media_session)
    : media_session_(media_session) {}

AudioFocusDelegateDefault::~AudioFocusDelegateDefault() = default;

bool AudioFocusDelegateDefault::RequestAudioFocus(
    AudioFocusManager::AudioFocusType audio_focus_type) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAudioFocus)) {
    return true;
  }

  AudioFocusManager::GetInstance()->RequestAudioFocus(media_session_,
                                                      audio_focus_type);
  return true;
}

void AudioFocusDelegateDefault::AbandonAudioFocus() {
  AudioFocusManager::GetInstance()->AbandonAudioFocus(media_session_);
}

// static
std::unique_ptr<AudioFocusDelegate> AudioFocusDelegate::Create(
    MediaSessionImpl* media_session) {
  return std::unique_ptr<AudioFocusDelegate>(
      new AudioFocusDelegateDefault(media_session));
}

}  // namespace content
