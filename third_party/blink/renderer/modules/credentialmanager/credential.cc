// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/credentialmanager/credential.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"

namespace blink {

Credential::~Credential() = default;

Credential::Credential(const String& id, const String& type)
    : id_(id), type_(type) {
  DCHECK(!id_.IsEmpty());
  DCHECK(!type_.IsEmpty());
}

KURL Credential::ParseStringAsURLOrThrow(const String& url,
                                         ExceptionState& exception_state) {
  if (url.IsEmpty())
    return KURL();
  KURL parsed_url = KURL(NullURL(), url);
  if (!parsed_url.IsValid()) {
    exception_state.ThrowDOMException(kSyntaxError,
                                      "'" + url + "' is not a valid URL.");
  }
  return parsed_url;
}

void Credential::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
