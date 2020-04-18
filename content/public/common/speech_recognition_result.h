// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_H_
#define CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_H_

#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"

namespace content {

struct SpeechRecognitionHypothesis {
  base::string16 utterance;
  double confidence;

  SpeechRecognitionHypothesis() : confidence(0.0) {}

  SpeechRecognitionHypothesis(const base::string16& utterance_value,
                              double confidence_value)
      : utterance(utterance_value),
        confidence(confidence_value) {
  }
};

typedef std::vector<SpeechRecognitionHypothesis>
    SpeechRecognitionHypothesisArray;

struct CONTENT_EXPORT SpeechRecognitionResult {
  SpeechRecognitionHypothesisArray hypotheses;
  bool is_provisional;

  SpeechRecognitionResult();
  SpeechRecognitionResult(const SpeechRecognitionResult& other);
  ~SpeechRecognitionResult();
};

typedef std::vector<SpeechRecognitionResult> SpeechRecognitionResults;

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_H_
