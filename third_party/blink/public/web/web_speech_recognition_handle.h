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

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_HANDLE_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_HANDLE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"

namespace blink {

class SpeechRecognition;

// WebSpeechRecognitionHandle is used by WebSpeechRecognizer to identify a
// recognition session, and by WebSpeechRecognizerClient to route
// recognition events.
class WebSpeechRecognitionHandle {
 public:
  ~WebSpeechRecognitionHandle() { Reset(); }
  WebSpeechRecognitionHandle() = default;

  WebSpeechRecognitionHandle(const WebSpeechRecognitionHandle& other) {
    Assign(other);
  }
  WebSpeechRecognitionHandle& operator=(
      const WebSpeechRecognitionHandle& other) {
    Assign(other);
    return *this;
  }

  BLINK_EXPORT void Reset();
  BLINK_EXPORT void Assign(const WebSpeechRecognitionHandle&);

  // Comparison functions are provided so that WebSpeechRecognitionHandle
  // objects can be stored in a hash map.
  BLINK_EXPORT bool Equals(const WebSpeechRecognitionHandle&) const;
  BLINK_EXPORT bool LessThan(const WebSpeechRecognitionHandle&) const;

#if INSIDE_BLINK
  WebSpeechRecognitionHandle(SpeechRecognition*);
  operator SpeechRecognition*() const;
#endif

 private:
  WebPrivatePtr<SpeechRecognition> private_;
};

inline bool operator==(const WebSpeechRecognitionHandle& a,
                       const WebSpeechRecognitionHandle& b) {
  return a.Equals(b);
}

inline bool operator!=(const WebSpeechRecognitionHandle& a,
                       const WebSpeechRecognitionHandle& b) {
  return !(a == b);
}

inline bool operator<(const WebSpeechRecognitionHandle& a,
                      const WebSpeechRecognitionHandle& b) {
  return a.LessThan(b);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_HANDLE_H_
