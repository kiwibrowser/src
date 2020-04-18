// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_SESSION_CONFIG_H_
#define CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_SESSION_CONFIG_H_

#include <stdint.h>

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/speech_recognition_session_context.h"
#include "content/public/browser/speech_recognition_session_preamble.h"
#include "content/public/common/speech_recognition_grammar.mojom.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/origin.h"

namespace content {

class SpeechRecognitionEventListener;

// Configuration params for creating a new speech recognition session.
struct CONTENT_EXPORT SpeechRecognitionSessionConfig {
  SpeechRecognitionSessionConfig();
  SpeechRecognitionSessionConfig(const SpeechRecognitionSessionConfig& other);
  ~SpeechRecognitionSessionConfig();

  std::string language;
  std::vector<mojom::SpeechRecognitionGrammar> grammars;
  url::Origin origin;
  bool filter_profanities;
  bool continuous;
  bool interim_results;
  uint32_t max_hypotheses;
  std::string auth_token;
  std::string auth_scope;
  scoped_refptr<SpeechRecognitionSessionPreamble> preamble;
  SpeechRecognitionSessionContext initial_context;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter;
  base::WeakPtr<SpeechRecognitionEventListener> event_listener;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_SESSION_CONFIG_H_
