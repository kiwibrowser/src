// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_AUDIO_SESSION_CONTROLLER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_AUDIO_SESSION_CONTROLLER_H_

#include "base/macros.h"

// The modes in which the audio session can be configured.
enum class AudioSessionMode : bool { PLAYBACK, RECORDING };

class AudioSessionController {
 public:
  AudioSessionController() = default;
  virtual ~AudioSessionController() = default;

  // Initializes the audio session for the current audio session mode.
  virtual void InitializeSessionIfNecessary() = 0;

  // Getter and setter for the audio session mode.  Setting to a new value will
  // configure the audio session for that mode.
  virtual AudioSessionMode GetSessionMode() = 0;
  virtual void SetSessionMode(AudioSessionMode mode) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioSessionController);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_AUDIO_SESSION_CONTROLLER_H_
