// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// PLEASE NOTE: this is a copy with modifications from chrome/browser/speech.
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#include "chromecast/browser/tts/tts_platform.h"

#include <string>

bool TtsPlatformImpl::LoadBuiltInTtsExtension(
    content::BrowserContext* browser_context) {
  return false;
}

std::string TtsPlatformImpl::error() {
  return error_;
}

void TtsPlatformImpl::clear_error() {
  error_ = std::string();
}

void TtsPlatformImpl::set_error(const std::string& error) {
  error_ = error;
}

void TtsPlatformImpl::WillSpeakUtteranceWithVoice(const Utterance* utterance,
                                                  const VoiceData& voice_data) {
}
