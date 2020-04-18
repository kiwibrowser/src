// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_STRUCT_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_STRUCT_TRAITS_H_

#include "content/public/common/speech_recognition_result.h"
#include "content/public/common/speech_recognition_result.mojom.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct StructTraits<content::mojom::SpeechRecognitionHypothesisDataView,
                    content::SpeechRecognitionHypothesis> {
  static const base::string16& utterance(
      const content::SpeechRecognitionHypothesis& r) {
    return r.utterance;
  }
  static double confidence(const content::SpeechRecognitionHypothesis& r) {
    return r.confidence;
  }
  static bool Read(content::mojom::SpeechRecognitionHypothesisDataView data,
                   content::SpeechRecognitionHypothesis* out);
};

template <>
struct StructTraits<content::mojom::SpeechRecognitionResultDataView,
                    content::SpeechRecognitionResult> {
  static const std::vector<content::SpeechRecognitionHypothesis>& hypotheses(
      const content::SpeechRecognitionResult& r) {
    return r.hypotheses;
  }

  static bool is_provisional(const content::SpeechRecognitionResult& r) {
    return r.is_provisional;
  }

  static bool Read(content::mojom::SpeechRecognitionResultDataView data,
                   content::SpeechRecognitionResult* out);
};

}  // namespace mojo

#endif  // CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_RESULT_STRUCT_TRAITS_H_
