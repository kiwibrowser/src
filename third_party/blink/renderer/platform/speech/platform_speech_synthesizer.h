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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIZER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_voice.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

enum SpeechBoundary { kSpeechWordBoundary, kSpeechSentenceBoundary };

class PlatformSpeechSynthesisUtterance;
class WebSpeechSynthesizer;
class WebSpeechSynthesizerClientImpl;

class PLATFORM_EXPORT PlatformSpeechSynthesizerClient
    : public GarbageCollectedMixin {
 public:
  virtual void DidStartSpeaking(PlatformSpeechSynthesisUtterance*) = 0;
  virtual void DidFinishSpeaking(PlatformSpeechSynthesisUtterance*) = 0;
  virtual void DidPauseSpeaking(PlatformSpeechSynthesisUtterance*) = 0;
  virtual void DidResumeSpeaking(PlatformSpeechSynthesisUtterance*) = 0;
  virtual void SpeakingErrorOccurred(PlatformSpeechSynthesisUtterance*) = 0;
  virtual void BoundaryEventOccurred(PlatformSpeechSynthesisUtterance*,
                                     SpeechBoundary,
                                     unsigned char_index) = 0;
  virtual void VoicesDidChange() = 0;

 protected:
  virtual ~PlatformSpeechSynthesizerClient() = default;
};

class PLATFORM_EXPORT PlatformSpeechSynthesizer
    : public GarbageCollectedFinalized<PlatformSpeechSynthesizer> {
 public:
  static PlatformSpeechSynthesizer* Create(PlatformSpeechSynthesizerClient*);

  virtual ~PlatformSpeechSynthesizer();

  virtual void Speak(PlatformSpeechSynthesisUtterance*);
  virtual void Pause();
  virtual void Resume();
  virtual void Cancel();

  PlatformSpeechSynthesizerClient* Client() const {
    return speech_synthesizer_client_;
  }

  const Vector<scoped_refptr<PlatformSpeechSynthesisVoice>>& GetVoiceList();

  void SetVoiceList(Vector<scoped_refptr<PlatformSpeechSynthesisVoice>>&);

  // Eager finalization is required to promptly release the owned
  // WebSpeechSynthesizer.
  //
  // If not and delayed until lazily swept, m_webSpeechSynthesizerClient may end
  // up being lazily swept first (i.e., before this PlatformSpeechSynthesizer),
  // leaving m_webSpeechSynthesizer with a dangling pointer to a finalized
  // object -- WebSpeechSynthesizer embedder implementations calling
  // notification methods in the other directions by way of
  // m_webSpeechSynthesizerClient. Eagerly releasing WebSpeechSynthesizer
  // prevents such unsafe accesses.
  EAGERLY_FINALIZE();
  virtual void Trace(blink::Visitor*);

 protected:
  explicit PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient*);

  virtual void InitializeVoiceList();

  Vector<scoped_refptr<PlatformSpeechSynthesisVoice>> voice_list_;
  bool voices_initialized_ = false;

 private:
  void MaybeInitializeVoiceList();

  Member<PlatformSpeechSynthesizerClient> speech_synthesizer_client_;

  std::unique_ptr<WebSpeechSynthesizer> web_speech_synthesizer_;
  Member<WebSpeechSynthesizerClientImpl> web_speech_synthesizer_client_;

  DISALLOW_COPY_AND_ASSIGN(PlatformSpeechSynthesizer);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIZER_H_
