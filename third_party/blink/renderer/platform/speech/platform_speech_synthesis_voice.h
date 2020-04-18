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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIS_VOICE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIS_VOICE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class PLATFORM_EXPORT PlatformSpeechSynthesisVoice final
    : public RefCounted<PlatformSpeechSynthesisVoice> {
 public:
  static scoped_refptr<PlatformSpeechSynthesisVoice> Create(
      const String& voice_uri,
      const String& name,
      const String& lang,
      bool local_service,
      bool is_default);
  static scoped_refptr<PlatformSpeechSynthesisVoice> Create();

  const String& VoiceURI() const { return voice_uri_; }
  void SetVoiceURI(const String& voice_uri) { voice_uri_ = voice_uri; }

  const String& GetName() const { return name_; }
  void SetName(const String& name) { name_ = name; }

  const String& Lang() const { return lang_; }
  void SetLang(const String& lang) { lang_ = lang; }

  bool LocalService() const { return local_service_; }
  void SetLocalService(bool local_service) { local_service_ = local_service; }

  bool IsDefault() const { return default_; }
  void SetIsDefault(bool is_default) { default_ = is_default; }

 private:
  PlatformSpeechSynthesisVoice(const String& voice_uri,
                               const String& name,
                               const String& lang,
                               bool local_service,
                               bool is_default);
  PlatformSpeechSynthesisVoice();

  String voice_uri_;
  String name_;
  String lang_;
  bool local_service_;
  bool default_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SPEECH_PLATFORM_SPEECH_SYNTHESIS_VOICE_H_
