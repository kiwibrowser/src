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

#include "third_party/blink/renderer/platform/exported/web_speech_synthesizer_client_impl.h"

#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_utterance.h"

namespace blink {

WebSpeechSynthesizerClientImpl::WebSpeechSynthesizerClientImpl(
    PlatformSpeechSynthesizer* synthesizer,
    PlatformSpeechSynthesizerClient* client)
    : synthesizer_(synthesizer), client_(client) {}

WebSpeechSynthesizerClientImpl::~WebSpeechSynthesizerClientImpl() = default;

void WebSpeechSynthesizerClientImpl::SetVoiceList(
    const WebVector<WebSpeechSynthesisVoice>& voices) {
  Vector<scoped_refptr<PlatformSpeechSynthesisVoice>> out_voices;
  for (size_t i = 0; i < voices.size(); i++)
    out_voices.push_back(voices[i]);
  synthesizer_->SetVoiceList(out_voices);
  client_->VoicesDidChange();
}

void WebSpeechSynthesizerClientImpl::DidStartSpeaking(
    const WebSpeechSynthesisUtterance& utterance) {
  client_->DidStartSpeaking(utterance);
}

void WebSpeechSynthesizerClientImpl::DidFinishSpeaking(
    const WebSpeechSynthesisUtterance& utterance) {
  client_->DidFinishSpeaking(utterance);
}

void WebSpeechSynthesizerClientImpl::DidPauseSpeaking(
    const WebSpeechSynthesisUtterance& utterance) {
  client_->DidPauseSpeaking(utterance);
}

void WebSpeechSynthesizerClientImpl::DidResumeSpeaking(
    const WebSpeechSynthesisUtterance& utterance) {
  client_->DidResumeSpeaking(utterance);
}

void WebSpeechSynthesizerClientImpl::SpeakingErrorOccurred(
    const WebSpeechSynthesisUtterance& utterance) {
  client_->SpeakingErrorOccurred(utterance);
}

void WebSpeechSynthesizerClientImpl::WordBoundaryEventOccurred(
    const WebSpeechSynthesisUtterance& utterance,
    unsigned char_index) {
  client_->BoundaryEventOccurred(utterance, kSpeechWordBoundary, char_index);
}

void WebSpeechSynthesizerClientImpl::SentenceBoundaryEventOccurred(
    const WebSpeechSynthesisUtterance& utterance,
    unsigned char_index) {
  client_->BoundaryEventOccurred(utterance, kSpeechSentenceBoundary,
                                 char_index);
}

void WebSpeechSynthesizerClientImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(synthesizer_);
  visitor->Trace(client_);
}

}  // namespace blink
