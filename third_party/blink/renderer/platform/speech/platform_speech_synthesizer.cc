/*
 * Copyright (C) 2013 Apple Computer, Inc.  All rights reserved.
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

#include "third_party/blink/renderer/platform/speech/platform_speech_synthesizer.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_speech_synthesis_utterance.h"
#include "third_party/blink/public/platform/web_speech_synthesizer.h"
#include "third_party/blink/public/platform/web_speech_synthesizer_client.h"
#include "third_party/blink/renderer/platform/exported/web_speech_synthesizer_client_impl.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_utterance.h"

namespace blink {

PlatformSpeechSynthesizer* PlatformSpeechSynthesizer::Create(
    PlatformSpeechSynthesizerClient* client) {
  PlatformSpeechSynthesizer* synthesizer =
      new PlatformSpeechSynthesizer(client);
#if defined(OS_ANDROID)
// On Android devices we don't fetch voices until the object
// is touched to avoid needlessly binding to TTS service, see
// https://crbug.com/811929.
#else
  synthesizer->InitializeVoiceList();
#endif
  return synthesizer;
}

PlatformSpeechSynthesizer::PlatformSpeechSynthesizer(
    PlatformSpeechSynthesizerClient* client)
    : speech_synthesizer_client_(client) {
  web_speech_synthesizer_client_ =
      new WebSpeechSynthesizerClientImpl(this, client);
  web_speech_synthesizer_ = Platform::Current()->CreateSpeechSynthesizer(
      web_speech_synthesizer_client_);
}

PlatformSpeechSynthesizer::~PlatformSpeechSynthesizer() = default;

void PlatformSpeechSynthesizer::Speak(
    PlatformSpeechSynthesisUtterance* utterance) {
  MaybeInitializeVoiceList();
  if (web_speech_synthesizer_ && web_speech_synthesizer_client_)
    web_speech_synthesizer_->Speak(WebSpeechSynthesisUtterance(utterance));
}

void PlatformSpeechSynthesizer::Pause() {
  MaybeInitializeVoiceList();
  if (web_speech_synthesizer_)
    web_speech_synthesizer_->Pause();
}

void PlatformSpeechSynthesizer::Resume() {
  MaybeInitializeVoiceList();
  if (web_speech_synthesizer_)
    web_speech_synthesizer_->Resume();
}

void PlatformSpeechSynthesizer::Cancel() {
  MaybeInitializeVoiceList();
  if (web_speech_synthesizer_)
    web_speech_synthesizer_->Cancel();
}

const Vector<scoped_refptr<PlatformSpeechSynthesisVoice>>&
PlatformSpeechSynthesizer::GetVoiceList() {
  MaybeInitializeVoiceList();
  return voice_list_;
}

void PlatformSpeechSynthesizer::SetVoiceList(
    Vector<scoped_refptr<PlatformSpeechSynthesisVoice>>& voices) {
  voice_list_ = voices;
}

void PlatformSpeechSynthesizer::MaybeInitializeVoiceList() {
  if (!voices_initialized_)
    InitializeVoiceList();
}

void PlatformSpeechSynthesizer::InitializeVoiceList() {
  if (web_speech_synthesizer_) {
    voices_initialized_ = true;
    web_speech_synthesizer_->UpdateVoiceList();
  }
}

void PlatformSpeechSynthesizer::Trace(blink::Visitor* visitor) {
  visitor->Trace(speech_synthesizer_client_);
  visitor->Trace(web_speech_synthesizer_client_);
}

}  // namespace blink
