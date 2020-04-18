// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_TEST_AUDIO_SESSION_CONTROLLER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_TEST_AUDIO_SESSION_CONTROLLER_H_

#include "base/macros.h"
#include "ios/public/provider/chrome/browser/voice/audio_session_controller.h"

// No-op AudioSessionController for use in tests.
class TestAudioSessionController : public AudioSessionController {
 public:
  TestAudioSessionController() = default;
  ~TestAudioSessionController() override = default;

  // AudioSessionController:
  void InitializeSessionIfNecessary() override;
  AudioSessionMode GetSessionMode() override;
  void SetSessionMode(AudioSessionMode mode) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAudioSessionController);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_TEST_AUDIO_SESSION_CONTROLLER_H_
