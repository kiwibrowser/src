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

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_PARAMS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_PARAMS_H_

#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_speech_grammar.h"

namespace blink {

class WebSpeechGrammar;

class WebSpeechRecognitionParams {
 public:
  WebSpeechRecognitionParams(const WebVector<WebSpeechGrammar>& grammars,
                             const WebString& language,
                             bool continuous,
                             bool interim_results,
                             unsigned long max_alternatives,
                             const WebSecurityOrigin& origin)
      : grammars_(grammars),
        language_(language),
        continuous_(continuous),
        interim_results_(interim_results),
        max_alternatives_(max_alternatives),
        origin_(origin) {}

  const WebVector<WebSpeechGrammar>& Grammars() const { return grammars_; }
  const WebString& Language() const { return language_; }
  bool Continuous() const { return continuous_; }
  bool InterimResults() const { return interim_results_; }
  unsigned long MaxAlternatives() const { return max_alternatives_; }
  const WebSecurityOrigin& Origin() const { return origin_; }

 private:
  WebVector<WebSpeechGrammar> grammars_;
  WebString language_;
  bool continuous_;
  bool interim_results_;
  unsigned long max_alternatives_;
  WebSecurityOrigin origin_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SPEECH_RECOGNITION_PARAMS_H_
