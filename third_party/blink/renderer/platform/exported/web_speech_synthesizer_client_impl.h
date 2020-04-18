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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_EXPORTED_WEB_SPEECH_SYNTHESIZER_CLIENT_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_EXPORTED_WEB_SPEECH_SYNTHESIZER_CLIENT_IMPL_H_

#include "third_party/blink/public/platform/web_speech_synthesis_utterance.h"
#include "third_party/blink/public/platform/web_speech_synthesis_voice.h"
#include "third_party/blink/public/platform/web_speech_synthesizer_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesizer.h"

namespace blink {

class PlatformSpeechSynthesizer;
class PlatformSpeechSynthesizerClient;

class WebSpeechSynthesizerClientImpl final
    : public GarbageCollectedFinalized<WebSpeechSynthesizerClientImpl>,
      public WebSpeechSynthesizerClient {
 public:
  WebSpeechSynthesizerClientImpl(PlatformSpeechSynthesizer*,
                                 PlatformSpeechSynthesizerClient*);
  ~WebSpeechSynthesizerClientImpl() override;

  void SetVoiceList(const WebVector<WebSpeechSynthesisVoice>& voices) override;
  void DidStartSpeaking(const WebSpeechSynthesisUtterance&) override;
  void DidFinishSpeaking(const WebSpeechSynthesisUtterance&) override;
  void DidPauseSpeaking(const WebSpeechSynthesisUtterance&) override;
  void DidResumeSpeaking(const WebSpeechSynthesisUtterance&) override;
  void SpeakingErrorOccurred(const WebSpeechSynthesisUtterance&) override;
  void WordBoundaryEventOccurred(const WebSpeechSynthesisUtterance&,
                                 unsigned char_index) override;
  void SentenceBoundaryEventOccurred(const WebSpeechSynthesisUtterance&,
                                     unsigned char_index) override;

  void Trace(blink::Visitor*);

 private:
  Member<PlatformSpeechSynthesizer> synthesizer_;
  Member<PlatformSpeechSynthesizerClient> client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_EXPORTED_WEB_SPEECH_SYNTHESIZER_CLIENT_IMPL_H_
