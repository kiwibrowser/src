/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_speech_recognizer_client.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_error.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

static String ErrorCodeToString(SpeechRecognitionError::ErrorCode code) {
  switch (code) {
    case SpeechRecognitionError::kErrorCodeOther:
      return "other";
    case SpeechRecognitionError::kErrorCodeNoSpeech:
      return "no-speech";
    case SpeechRecognitionError::kErrorCodeAborted:
      return "aborted";
    case SpeechRecognitionError::kErrorCodeAudioCapture:
      return "audio-capture";
    case SpeechRecognitionError::kErrorCodeNetwork:
      return "network";
    case SpeechRecognitionError::kErrorCodeNotAllowed:
      return "not-allowed";
    case SpeechRecognitionError::kErrorCodeServiceNotAllowed:
      return "service-not-allowed";
    case SpeechRecognitionError::kErrorCodeBadGrammar:
      return "bad-grammar";
    case SpeechRecognitionError::kErrorCodeLanguageNotSupported:
      return "language-not-supported";
  }

  NOTREACHED();
  return String();
}

SpeechRecognitionError* SpeechRecognitionError::Create(ErrorCode code,
                                                       const String& message) {
  return new SpeechRecognitionError(ErrorCodeToString(code), message);
}

SpeechRecognitionError* SpeechRecognitionError::Create(
    const AtomicString& event_name,
    const SpeechRecognitionErrorInit& initializer) {
  return new SpeechRecognitionError(event_name, initializer);
}

SpeechRecognitionError::SpeechRecognitionError(const String& error,
                                               const String& message)
    : Event(EventTypeNames::error, Bubbles::kNo, Cancelable::kNo),
      error_(error),
      message_(message) {}

SpeechRecognitionError::SpeechRecognitionError(
    const AtomicString& event_name,
    const SpeechRecognitionErrorInit& initializer)
    : Event(event_name, initializer) {
  if (initializer.hasError())
    error_ = initializer.error();
  if (initializer.hasMessage())
    message_ = initializer.message();
}

const AtomicString& SpeechRecognitionError::InterfaceName() const {
  return EventNames::SpeechRecognitionError;
}

STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kOtherError,
                   SpeechRecognitionError::kErrorCodeOther);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kNoSpeechError,
                   SpeechRecognitionError::kErrorCodeNoSpeech);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kAbortedError,
                   SpeechRecognitionError::kErrorCodeAborted);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kAudioCaptureError,
                   SpeechRecognitionError::kErrorCodeAudioCapture);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kNetworkError,
                   SpeechRecognitionError::kErrorCodeNetwork);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kNotAllowedError,
                   SpeechRecognitionError::kErrorCodeNotAllowed);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kServiceNotAllowedError,
                   SpeechRecognitionError::kErrorCodeServiceNotAllowed);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kBadGrammarError,
                   SpeechRecognitionError::kErrorCodeBadGrammar);
STATIC_ASSERT_ENUM(WebSpeechRecognizerClient::kLanguageNotSupportedError,
                   SpeechRecognitionError::kErrorCodeLanguageNotSupported);

}  // namespace blink
