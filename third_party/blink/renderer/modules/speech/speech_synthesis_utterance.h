/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_UTTERANCE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_UTTERANCE_H_

#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/speech/speech_synthesis_voice.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_utterance.h"

namespace blink {

class SpeechSynthesisUtterance final
    : public EventTargetWithInlineData,
      public ContextClient,
      public PlatformSpeechSynthesisUtteranceClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(SpeechSynthesisUtterance);

 public:
  static SpeechSynthesisUtterance* Create(ExecutionContext*, const String&);

  ~SpeechSynthesisUtterance() override;

  const String& text() const { return platform_utterance_->GetText(); }
  void setText(const String& text) { platform_utterance_->SetText(text); }

  const String& lang() const { return platform_utterance_->Lang(); }
  void setLang(const String& lang) { platform_utterance_->SetLang(lang); }

  SpeechSynthesisVoice* voice() const;
  void setVoice(SpeechSynthesisVoice*);

  float volume() const { return platform_utterance_->Volume(); }
  void setVolume(float volume) { platform_utterance_->SetVolume(volume); }

  float rate() const { return platform_utterance_->Rate(); }
  void setRate(float rate) { platform_utterance_->SetRate(rate); }

  float pitch() const { return platform_utterance_->Pitch(); }
  void setPitch(float pitch) { platform_utterance_->SetPitch(pitch); }

  double StartTime() const { return platform_utterance_->StartTime(); }
  void SetStartTime(double start_time) {
    platform_utterance_->SetStartTime(start_time);
  }

  DEFINE_ATTRIBUTE_EVENT_LISTENER(start);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(end);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(pause);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(resume);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(mark);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(boundary);

  ExecutionContext* GetExecutionContext() const override {
    return ContextClient::GetExecutionContext();
  }

  PlatformSpeechSynthesisUtterance* PlatformUtterance() const {
    return platform_utterance_;
  }

  void Trace(blink::Visitor*) override;

 private:
  SpeechSynthesisUtterance(ExecutionContext*, const String&);

  // EventTarget
  const AtomicString& InterfaceName() const override;

  Member<PlatformSpeechSynthesisUtterance> platform_utterance_;
  Member<SpeechSynthesisVoice> voice_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_UTTERANCE_H_
