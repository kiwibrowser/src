// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_H_
#define CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_H_

#include "content/common/content_export.h"

namespace content {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content_public.common
// GENERATED_JAVA_PREFIX_TO_STRIP: SPEECH_RECOGNITION_ERROR_
enum SpeechRecognitionErrorCode {
  // There was no error.
  SPEECH_RECOGNITION_ERROR_NONE,

  // No speech heard before timeout.
  SPEECH_RECOGNITION_ERROR_NO_SPEECH,

  // The user or a script aborted speech input.
  SPEECH_RECOGNITION_ERROR_ABORTED,

  // There was an error with recording audio.
  SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE,

  // There was a network error.
  SPEECH_RECOGNITION_ERROR_NETWORK,

  // Not allowed for privacy or security reasons.
  SPEECH_RECOGNITION_ERROR_NOT_ALLOWED,

  // Speech service is not allowed for privacy or security reasons.
  SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED,

  // There was an error in the speech recognition grammar.
  SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR,

  // The language was not supported.
  SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED,

  // Speech was heard, but could not be interpreted.
  SPEECH_RECOGNITION_ERROR_NO_MATCH,

  SPEECH_RECOGNITION_ERROR_LAST = SPEECH_RECOGNITION_ERROR_NO_MATCH,
};

// Error details for the SPEECH_RECOGNITION_ERROR_AUDIO error.
enum SpeechAudioErrorDetails {
  SPEECH_AUDIO_ERROR_DETAILS_NONE = 0,
  SPEECH_AUDIO_ERROR_DETAILS_NO_MIC,
  SPEECH_AUDIO_ERROR_DETAILS_LAST = SPEECH_AUDIO_ERROR_DETAILS_NO_MIC
};

struct CONTENT_EXPORT SpeechRecognitionError {
  SpeechRecognitionErrorCode code;
  SpeechAudioErrorDetails details;

  SpeechRecognitionError()
      : code(SPEECH_RECOGNITION_ERROR_NONE),
        details(SPEECH_AUDIO_ERROR_DETAILS_NONE) {
  }
  explicit SpeechRecognitionError(SpeechRecognitionErrorCode code_value)
      : code(code_value),
        details(SPEECH_AUDIO_ERROR_DETAILS_NONE) {
  }
  SpeechRecognitionError(SpeechRecognitionErrorCode code_value,
                         SpeechAudioErrorDetails details_value)
      : code(code_value),
        details(details_value) {
  }
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_ERROR_H_
