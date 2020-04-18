// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/speech_recognition_result_struct_traits.h"

namespace mojo {

bool StructTraits<content::mojom::SpeechRecognitionHypothesisDataView,
                  content::SpeechRecognitionHypothesis>::
    Read(content::mojom::SpeechRecognitionHypothesisDataView data,
         content::SpeechRecognitionHypothesis* out) {
  if (!data.ReadUtterance(&out->utterance))
    return false;
  out->confidence = data.confidence();
  return true;
}

bool StructTraits<content::mojom::SpeechRecognitionResultDataView,
                  content::SpeechRecognitionResult>::
    Read(content::mojom::SpeechRecognitionResultDataView data,
         content::SpeechRecognitionResult* out) {
  if (!data.ReadHypotheses(&out->hypotheses))
    return false;
  out->is_provisional = data.is_provisional();
  return true;
}

}  // namespace mojo
