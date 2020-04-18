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

#include "third_party/blink/public/web/web_speech_recognition_result.h"

#include "third_party/blink/renderer/modules/speech/speech_recognition_alternative.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_result.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

void WebSpeechRecognitionResult::Assign(
    const WebSpeechRecognitionResult& other) {
  private_ = other.private_;
}

void WebSpeechRecognitionResult::Assign(const WebVector<WebString>& transcripts,
                                        const WebVector<float>& confidences,
                                        bool final) {
  DCHECK_EQ(transcripts.size(), confidences.size());

  HeapVector<Member<SpeechRecognitionAlternative>> alternatives(
      transcripts.size());
  for (size_t i = 0; i < transcripts.size(); ++i) {
    alternatives[i] =
        SpeechRecognitionAlternative::Create(transcripts[i], confidences[i]);
  }

  private_ = SpeechRecognitionResult::Create(alternatives, final);
}

void WebSpeechRecognitionResult::Reset() {
  private_.Reset();
}

WebSpeechRecognitionResult::operator SpeechRecognitionResult*() const {
  return private_.Get();
}

}  // namespace blink
