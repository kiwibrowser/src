// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/trustedtypes/trusted_script_url.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

TrustedScriptURL::TrustedScriptURL(const KURL& url) : url_(url) {}

TrustedScriptURL* TrustedScriptURL::unsafelyCreate(ScriptState* script_state,
                                                   const String& url) {
  return TrustedScriptURL::Create(
      ExecutionContext::From(script_state)->CompleteURL(url));
}

String TrustedScriptURL::toString() const {
  return url_.GetString();
}

}  // namespace blink
