// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_FETCH_REQUEST_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_FETCH_REQUEST_H_

#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// A ModuleScriptFetchRequest essentially serves as a "parameter object" for
// Modulator::Fetch{Single,NewSingle}.
class ModuleScriptFetchRequest final {
  STACK_ALLOCATED();

 public:
  // Referrer is set only for internal module script fetch algorithms triggered
  // from ModuleTreeLinker to fetch descendant module scripts.
  ModuleScriptFetchRequest(const KURL& url,
                           WebURLRequest::RequestContext destination,
                           const ScriptFetchOptions& options,
                           const String& referrer,
                           ReferrerPolicy referrer_policy,
                           const TextPosition& referrer_position)
      : url_(url),
        destination_(destination),
        options_(options),
        referrer_(referrer),
        referrer_policy_(referrer_policy),
        referrer_position_(referrer_position) {}

  static ModuleScriptFetchRequest CreateForTest(const KURL& url) {
    return ModuleScriptFetchRequest(
        url, WebURLRequest::kRequestContextScript, ScriptFetchOptions(),
        Referrer::NoReferrer(), kReferrerPolicyDefault,
        TextPosition::MinimumPosition());
  }
  ~ModuleScriptFetchRequest() = default;

  const KURL& Url() const { return url_; }
  WebURLRequest::RequestContext Destination() const { return destination_; }
  const ScriptFetchOptions& Options() const { return options_; }
  const AtomicString& GetReferrer() const { return referrer_; }
  ReferrerPolicy GetReferrerPolicy() const { return referrer_policy_; }
  const TextPosition& GetReferrerPosition() const { return referrer_position_; }

 private:
  const KURL url_;
  const WebURLRequest::RequestContext destination_;
  const ScriptFetchOptions options_;
  const AtomicString referrer_;
  const ReferrerPolicy referrer_policy_;
  const TextPosition referrer_position_;
};

}  // namespace blink

#endif
