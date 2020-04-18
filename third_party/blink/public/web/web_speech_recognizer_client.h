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

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNIZER_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNIZER_CLIENT_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

class SpeechRecognitionClientProxy;
class WebSpeechRecognitionResult;
class WebSpeechRecognitionHandle;
class WebString;

// A client for reporting progress on speech recognition for a specific handle.
class BLINK_EXPORT WebSpeechRecognizerClient {
 public:
  enum ErrorCode {
    kOtherError = 0,
    kNoSpeechError = 1,
    kAbortedError = 2,
    kAudioCaptureError = 3,
    kNetworkError = 4,
    kNotAllowedError = 5,
    kServiceNotAllowedError = 6,
    kBadGrammarError = 7,
    kLanguageNotSupportedError = 8
  };

  WebSpeechRecognizerClient() = default;
  WebSpeechRecognizerClient(const WebSpeechRecognizerClient& other) {
    Assign(other);
  }
  WebSpeechRecognizerClient& operator=(const WebSpeechRecognizerClient& other) {
    Assign(other);
    return *this;
  }
  ~WebSpeechRecognizerClient();

  bool operator==(const WebSpeechRecognizerClient& other) const;
  bool operator!=(const WebSpeechRecognizerClient& other) const;

  void Reset();
  bool IsNull() const { return private_.IsNull(); }

  // These methods correspond to the events described in the spec:
  // http://speech-javascript-api-spec.googlecode.com/git/speechapi.html#speechreco-events

  // To be called when the embedder has started to capture audio.
  void DidStartAudio(const WebSpeechRecognitionHandle&);

  // To be called when some sound, possibly speech, has been detected.
  // This is expected to be called after didStartAudio.
  void DidStartSound(const WebSpeechRecognitionHandle&);

  // To be called when sound is no longer detected.
  // This is expected to be called after didEndSpeech.
  void DidEndSound(const WebSpeechRecognitionHandle&);

  // To be called when audio capture has stopped.
  // This is expected to be called after didEndSound.
  void DidEndAudio(const WebSpeechRecognitionHandle&);

  // To be called when the speech recognizer provides new results.
  // - newFinalResults contains zero or more final results that are new since
  // the last time the function was called.
  // - currentInterimResults contains zero or more inteirm results that
  // replace the interim results that were reported the last time this
  // function was called.
  void DidReceiveResults(
      const WebSpeechRecognitionHandle&,
      const WebVector<WebSpeechRecognitionResult>& new_final_results,
      const WebVector<WebSpeechRecognitionResult>& current_interim_results);

  // To be called when the speech recognizer returns a final result with no
  // recognizion hypothesis.
  void DidReceiveNoMatch(const WebSpeechRecognitionHandle&,
                         const WebSpeechRecognitionResult&);

  // To be called when a speech recognition error occurs.
  void DidReceiveError(const WebSpeechRecognitionHandle&,
                       const WebString& message,
                       ErrorCode);

  // To be called when the recognizer has begun to listen to the audio with
  // the intention of recognizing.
  void DidStart(const WebSpeechRecognitionHandle&);

  // To be called when the recognition session has ended. This must always be
  // called, no matter the reason for the end.
  void DidEnd(const WebSpeechRecognitionHandle&);

#if INSIDE_BLINK
  explicit WebSpeechRecognizerClient(SpeechRecognitionClientProxy*);
#endif

 private:
  void Assign(const WebSpeechRecognizerClient&);

  WebPrivatePtr<SpeechRecognitionClientProxy> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNIZER_CLIENT_H_
