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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CONTROLLER_H_

#include <memory>

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_client.h"

namespace blink {

class SpeechRecognitionController final
    : public GarbageCollectedFinalized<SpeechRecognitionController>,
      public Supplement<LocalFrame> {
  USING_GARBAGE_COLLECTED_MIXIN(SpeechRecognitionController);

 public:
  static const char kSupplementName[];

  virtual ~SpeechRecognitionController();

  void Start(SpeechRecognition* recognition,
             const SpeechGrammarList* grammars,
             const String& lang,
             bool continuous,
             bool interim_results,
             unsigned long max_alternatives) {
    client_->Start(recognition, grammars, lang, continuous, interim_results,
                   max_alternatives);
  }

  void Stop(SpeechRecognition* recognition) { client_->Stop(recognition); }
  void Abort(SpeechRecognition* recognition) { client_->Abort(recognition); }

  static SpeechRecognitionController* Create(SpeechRecognitionClient*);
  static SpeechRecognitionController* From(LocalFrame* frame) {
    return Supplement<LocalFrame>::From<SpeechRecognitionController>(frame);
  }

  void Trace(blink::Visitor* visitor) override {
    Supplement<LocalFrame>::Trace(visitor);
    visitor->Trace(client_);
  }

 private:
  explicit SpeechRecognitionController(SpeechRecognitionClient*);

  Member<SpeechRecognitionClient> client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CONTROLLER_H_
