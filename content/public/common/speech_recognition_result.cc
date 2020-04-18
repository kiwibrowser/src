// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/speech_recognition_result.h"

namespace content {

SpeechRecognitionResult::SpeechRecognitionResult()
    : is_provisional(false) {
}

SpeechRecognitionResult::SpeechRecognitionResult(
    const SpeechRecognitionResult& other) = default;

SpeechRecognitionResult::~SpeechRecognitionResult() {
}

}  // namespace content

