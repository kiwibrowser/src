// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/public/provider/chrome/browser/voice/test_audio_session_controller.h"

void TestAudioSessionController::InitializeSessionIfNecessary() {}

AudioSessionMode TestAudioSessionController::GetSessionMode() {
  return AudioSessionMode::PLAYBACK;
}

void TestAudioSessionController::SetSessionMode(AudioSessionMode mode) {}
