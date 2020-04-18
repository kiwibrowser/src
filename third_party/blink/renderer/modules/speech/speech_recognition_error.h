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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_ERROR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_ERROR_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_error_init.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class MODULES_EXPORT SpeechRecognitionError final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum ErrorCode {
    // FIXME: This is an unspecified error and Chromium should stop using it.
    kErrorCodeOther = 0,

    kErrorCodeNoSpeech = 1,
    kErrorCodeAborted = 2,
    kErrorCodeAudioCapture = 3,
    kErrorCodeNetwork = 4,
    kErrorCodeNotAllowed = 5,
    kErrorCodeServiceNotAllowed = 6,
    kErrorCodeBadGrammar = 7,
    kErrorCodeLanguageNotSupported = 8
  };

  static SpeechRecognitionError* Create(ErrorCode, const String&);
  static SpeechRecognitionError* Create(const AtomicString&,
                                        const SpeechRecognitionErrorInit&);

  const String& error() { return error_; }
  const String& message() { return message_; }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor* visitor) override { Event::Trace(visitor); }

 private:
  SpeechRecognitionError(const String&, const String&);
  SpeechRecognitionError(const AtomicString&,
                         const SpeechRecognitionErrorInit&);

  String error_;
  String message_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_ERROR_H_
