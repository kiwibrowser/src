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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_UTTERANCE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_UTTERANCE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class PlatformSpeechSynthesisUtterance;

class WebSpeechSynthesisUtterance {
 public:
  WebSpeechSynthesisUtterance() = default;
  WebSpeechSynthesisUtterance(const WebSpeechSynthesisUtterance& other) {
    Assign(other);
  }
  ~WebSpeechSynthesisUtterance() { Reset(); }

  WebSpeechSynthesisUtterance& operator=(
      const WebSpeechSynthesisUtterance& other) {
    Assign(other);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Assign(const WebSpeechSynthesisUtterance&);
  BLINK_PLATFORM_EXPORT void Reset();
  bool IsNull() const { return private_.IsNull(); }

  BLINK_PLATFORM_EXPORT WebString GetText() const;
  BLINK_PLATFORM_EXPORT WebString Lang() const;
  BLINK_PLATFORM_EXPORT WebString Voice() const;

  // As defined in:
  // https://dvcs.w3.org/hg/speech-api/raw-file/tip/speechapi.html
  BLINK_PLATFORM_EXPORT float Volume() const;  // 0...1, 1 is default
  BLINK_PLATFORM_EXPORT float Rate() const;    // 0.1...10, 1 is default
  BLINK_PLATFORM_EXPORT float Pitch() const;   // 0...2, 1 is default

  BLINK_PLATFORM_EXPORT double StartTime() const;  // In seconds.

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebSpeechSynthesisUtterance(
      PlatformSpeechSynthesisUtterance*);
  BLINK_PLATFORM_EXPORT WebSpeechSynthesisUtterance& operator=(
      PlatformSpeechSynthesisUtterance*);
  BLINK_PLATFORM_EXPORT operator PlatformSpeechSynthesisUtterance*() const;
#endif

 private:
  WebPrivatePtr<PlatformSpeechSynthesisUtterance> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_UTTERANCE_H_
