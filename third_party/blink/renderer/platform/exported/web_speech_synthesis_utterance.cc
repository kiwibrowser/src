/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/platform/web_speech_synthesis_utterance.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_utterance.h"

namespace blink {

WebSpeechSynthesisUtterance::WebSpeechSynthesisUtterance(
    PlatformSpeechSynthesisUtterance* utterance)
    : private_(utterance) {}

WebSpeechSynthesisUtterance& WebSpeechSynthesisUtterance::operator=(
    PlatformSpeechSynthesisUtterance* utterance) {
  private_ = utterance;
  return *this;
}

void WebSpeechSynthesisUtterance::Assign(
    const WebSpeechSynthesisUtterance& other) {
  private_ = other.private_;
}

void WebSpeechSynthesisUtterance::Reset() {
  private_.Reset();
}

WebSpeechSynthesisUtterance::operator PlatformSpeechSynthesisUtterance*()
    const {
  return private_.Get();
}

WebString WebSpeechSynthesisUtterance::GetText() const {
  return private_->GetText();
}

WebString WebSpeechSynthesisUtterance::Lang() const {
  return private_->Lang();
}

WebString WebSpeechSynthesisUtterance::Voice() const {
  return private_->Voice() ? WebString(private_->Voice()->GetName())
                           : WebString();
}

float WebSpeechSynthesisUtterance::Volume() const {
  return private_->Volume();
}

float WebSpeechSynthesisUtterance::Rate() const {
  return private_->Rate();
}

float WebSpeechSynthesisUtterance::Pitch() const {
  return private_->Pitch();
}

double WebSpeechSynthesisUtterance::StartTime() const {
  return private_->StartTime();
}

}  // namespace blink
