// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_COMMON_TTS_UTTERANCE_REQUEST_H_
#define CHROMECAST_COMMON_TTS_UTTERANCE_REQUEST_H_

// PLEASE NOTE: this is a copy with modifications from chrome/common
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#include <vector>

#include "base/strings/string16.h"

struct TtsUtteranceRequest {
  TtsUtteranceRequest();
  ~TtsUtteranceRequest();

  int id;
  std::string text;
  std::string lang;
  std::string voice;
  float volume;
  float rate;
  float pitch;
};

struct TtsVoice {
  TtsVoice();
  TtsVoice(const TtsVoice& other);
  ~TtsVoice();

  std::string voice_uri;
  std::string name;
  std::string lang;
  bool local_service;
  bool is_default;
};

#endif  // CHROMECAST_COMMON_TTS_UTTERANCE_REQUEST_H_
