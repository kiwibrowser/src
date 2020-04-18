/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/public/platform/web_speech_synthesis_voice.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_voice.h"

namespace blink {

WebSpeechSynthesisVoice::WebSpeechSynthesisVoice()
    : private_(PlatformSpeechSynthesisVoice::Create()) {}

void WebSpeechSynthesisVoice::Assign(const WebSpeechSynthesisVoice& other) {
  private_ = other.private_;
}

void WebSpeechSynthesisVoice::Reset() {
  private_.Reset();
}

void WebSpeechSynthesisVoice::SetVoiceURI(const WebString& voice_uri) {
  private_->SetVoiceURI(voice_uri);
}

void WebSpeechSynthesisVoice::SetName(const WebString& name) {
  private_->SetName(name);
}

void WebSpeechSynthesisVoice::SetLanguage(const WebString& language) {
  private_->SetLang(language);
}

void WebSpeechSynthesisVoice::SetIsLocalService(bool is_local_service) {
  private_->SetLocalService(is_local_service);
}

void WebSpeechSynthesisVoice::SetIsDefault(bool is_default) {
  private_->SetIsDefault(is_default);
}

WebSpeechSynthesisVoice::operator PlatformSpeechSynthesisVoice*() const {
  return private_.Get();
}

}  // namespace blink
