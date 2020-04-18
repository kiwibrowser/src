// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// PLEASE NOTE: this is a copy with modifications from chrome/common
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#include "chromecast/common/tts_utterance_request.h"

TtsUtteranceRequest::TtsUtteranceRequest()
    : id(0), volume(1.0), rate(1.0), pitch(1.0) {}

TtsUtteranceRequest::~TtsUtteranceRequest() {}

TtsVoice::TtsVoice() : local_service(true), is_default(false) {}

TtsVoice::TtsVoice(const TtsVoice& other) = default;

TtsVoice::~TtsVoice() {}
