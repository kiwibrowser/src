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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CLIENT_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CLIENT_PROXY_H_

#include <memory>
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_speech_recognizer_client.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_client.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class WebSpeechRecognitionHandle;
class WebSpeechRecognitionResult;
class WebSpeechRecognizer;
class WebString;

class MODULES_EXPORT SpeechRecognitionClientProxy final
    : public SpeechRecognitionClient {
 public:
  ~SpeechRecognitionClientProxy() override;

  // Constructing a proxy object with a null WebSpeechRecognizer is safe in
  // itself, but attempting to call start/stop/abort on it will crash.
  static SpeechRecognitionClientProxy* Create(WebSpeechRecognizer*);

  // SpeechRecognitionClient:
  void Start(SpeechRecognition*,
             const SpeechGrammarList*,
             const String& lang,
             bool continuous,
             bool interim_results,
             unsigned long max_alternatives) override;
  void Stop(SpeechRecognition*) override;
  void Abort(SpeechRecognition*) override;

  void DidStartAudio(const WebSpeechRecognitionHandle&);
  void DidStartSound(const WebSpeechRecognitionHandle&);
  void DidEndSound(const WebSpeechRecognitionHandle&);
  void DidEndAudio(const WebSpeechRecognitionHandle&);
  void DidReceiveResults(
      const WebSpeechRecognitionHandle&,
      const WebVector<WebSpeechRecognitionResult>& new_final_results,
      const WebVector<WebSpeechRecognitionResult>& current_interim_results);
  void DidReceiveNoMatch(const WebSpeechRecognitionHandle&,
                         const WebSpeechRecognitionResult&);
  void DidReceiveError(const WebSpeechRecognitionHandle&,
                       const WebString& message,
                       WebSpeechRecognizerClient::ErrorCode);
  void DidStart(const WebSpeechRecognitionHandle&);
  void DidEnd(const WebSpeechRecognitionHandle&);

  // Required by garbage collection
  void Trace(blink::Visitor* visitor) override;

 private:
  explicit SpeechRecognitionClientProxy(WebSpeechRecognizer*);

  WebSpeechRecognizer* recognizer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_RECOGNITION_CLIENT_PROXY_H_
