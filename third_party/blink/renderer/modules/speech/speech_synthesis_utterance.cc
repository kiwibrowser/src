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

#include "third_party/blink/renderer/modules/speech/speech_synthesis_utterance.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {

SpeechSynthesisUtterance* SpeechSynthesisUtterance::Create(
    ExecutionContext* context,
    const String& text) {
  return new SpeechSynthesisUtterance(context, text);
}

SpeechSynthesisUtterance::SpeechSynthesisUtterance(ExecutionContext* context,
                                                   const String& text)
    : ContextClient(context),
      platform_utterance_(PlatformSpeechSynthesisUtterance::Create(this)) {
  platform_utterance_->SetText(text);
}

SpeechSynthesisUtterance::~SpeechSynthesisUtterance() = default;

const AtomicString& SpeechSynthesisUtterance::InterfaceName() const {
  return EventTargetNames::SpeechSynthesisUtterance;
}

SpeechSynthesisVoice* SpeechSynthesisUtterance::voice() const {
  return voice_;
}

void SpeechSynthesisUtterance::setVoice(SpeechSynthesisVoice* voice) {
  // Cache our own version of the SpeechSynthesisVoice so that we don't have to
  // do some lookup to go from the platform voice back to the speech synthesis
  // voice in the read property.
  voice_ = voice;

  if (voice)
    platform_utterance_->SetVoice(voice->PlatformVoice());
}

void SpeechSynthesisUtterance::Trace(blink::Visitor* visitor) {
  visitor->Trace(platform_utterance_);
  visitor->Trace(voice_);
  ContextClient::Trace(visitor);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
