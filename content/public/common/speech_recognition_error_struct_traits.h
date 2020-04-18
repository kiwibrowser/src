// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_STRUCT_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_STRUCT_TRAITS_H_

#include "content/public/common/speech_recognition_error.h"
#include "content/public/common/speech_recognition_error.mojom.h"

namespace mojo {

template <>
struct EnumTraits<content::mojom::SpeechRecognitionErrorCode,
                  content::SpeechRecognitionErrorCode> {
  static content::mojom::SpeechRecognitionErrorCode ToMojom(
      content::SpeechRecognitionErrorCode input) {
    switch (input) {
      case content::SpeechRecognitionErrorCode::SPEECH_RECOGNITION_ERROR_NONE:
        return content::mojom::SpeechRecognitionErrorCode::kNone;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_NO_SPEECH:
        return content::mojom::SpeechRecognitionErrorCode::kNoSpeech;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_ABORTED:
        return content::mojom::SpeechRecognitionErrorCode::kAborted;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE:
        return content::mojom::SpeechRecognitionErrorCode::kAudioCapture;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_NETWORK:
        return content::mojom::SpeechRecognitionErrorCode::kNetwork;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_NOT_ALLOWED:
        return content::mojom::SpeechRecognitionErrorCode::kNotAllowed;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED:
        return content::mojom::SpeechRecognitionErrorCode::kServiceNotAllowed;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR:
        return content::mojom::SpeechRecognitionErrorCode::kBadGrammar;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED:
        return content::mojom::SpeechRecognitionErrorCode::
            kLanguageNotSupported;
      case content::SpeechRecognitionErrorCode::
          SPEECH_RECOGNITION_ERROR_NO_MATCH:
        return content::mojom::SpeechRecognitionErrorCode::kNoMatch;
    }
    NOTREACHED();
    return content::mojom::SpeechRecognitionErrorCode::kNoMatch;
  }

  static bool FromMojom(content::mojom::SpeechRecognitionErrorCode input,
                        content::SpeechRecognitionErrorCode* output) {
    switch (input) {
      case content::mojom::SpeechRecognitionErrorCode::kNone:
        *output =
            content::SpeechRecognitionErrorCode::SPEECH_RECOGNITION_ERROR_NONE;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNoSpeech:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_NO_SPEECH;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kAborted:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_ABORTED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kAudioCapture:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNetwork:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_NETWORK;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNotAllowed:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_NOT_ALLOWED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kServiceNotAllowed:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kBadGrammar:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kLanguageNotSupported:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNoMatch:
        *output = content::SpeechRecognitionErrorCode::
            SPEECH_RECOGNITION_ERROR_NO_MATCH;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<content::mojom::SpeechAudioErrorDetails,
                  content::SpeechAudioErrorDetails> {
  static content::mojom::SpeechAudioErrorDetails ToMojom(
      content::SpeechAudioErrorDetails input) {
    switch (input) {
      case content::SpeechAudioErrorDetails::SPEECH_AUDIO_ERROR_DETAILS_NONE:
        return content::mojom::SpeechAudioErrorDetails::kNone;
      case content::SpeechAudioErrorDetails::SPEECH_AUDIO_ERROR_DETAILS_NO_MIC:
        return content::mojom::SpeechAudioErrorDetails::kNoMic;
    }
    NOTREACHED();
    return content::mojom::SpeechAudioErrorDetails::kNoMic;
  }

  static bool FromMojom(content::mojom::SpeechAudioErrorDetails input,
                        content::SpeechAudioErrorDetails* output) {
    switch (input) {
      case content::mojom::SpeechAudioErrorDetails::kNone:
        *output =
            content::SpeechAudioErrorDetails::SPEECH_AUDIO_ERROR_DETAILS_NONE;
        return true;
      case content::mojom::SpeechAudioErrorDetails::kNoMic:
        *output =
            content::SpeechAudioErrorDetails::SPEECH_AUDIO_ERROR_DETAILS_NO_MIC;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<content::mojom::SpeechRecognitionErrorDataView,
                    content::SpeechRecognitionError> {
  static content::SpeechRecognitionErrorCode code(
      const content::SpeechRecognitionError& r) {
    return r.code;
  }
  static content::SpeechAudioErrorDetails details(
      const content::SpeechRecognitionError& r) {
    return r.details;
  }
  static bool Read(content::mojom::SpeechRecognitionErrorDataView data,
                   content::SpeechRecognitionError* out);
};

}  // namespace mojo

#endif  // CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_STRUCT_TRAITS_H_
