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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_VOICE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_VOICE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class PlatformSpeechSynthesisVoice;

class WebSpeechSynthesisVoice {
 public:
  BLINK_PLATFORM_EXPORT WebSpeechSynthesisVoice();
  WebSpeechSynthesisVoice(const WebSpeechSynthesisVoice& other) {
    Assign(other);
  }
  ~WebSpeechSynthesisVoice() { Reset(); }

  WebSpeechSynthesisVoice& operator=(const WebSpeechSynthesisVoice& other) {
    Assign(other);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Assign(const WebSpeechSynthesisVoice&);
  BLINK_PLATFORM_EXPORT void Reset();

  BLINK_PLATFORM_EXPORT void SetVoiceURI(const WebString&);
  BLINK_PLATFORM_EXPORT void SetName(const WebString&);
  BLINK_PLATFORM_EXPORT void SetLanguage(const WebString&);
  BLINK_PLATFORM_EXPORT void SetIsLocalService(bool);
  BLINK_PLATFORM_EXPORT void SetIsDefault(bool);

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT operator PlatformSpeechSynthesisVoice*() const;
#endif

 private:
  WebPrivatePtr<PlatformSpeechSynthesisVoice> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SPEECH_SYNTHESIS_VOICE_H_
