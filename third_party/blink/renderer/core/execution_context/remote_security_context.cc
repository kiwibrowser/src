// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/execution_context/remote_security_context.h"

#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

RemoteSecurityContext::RemoteSecurityContext() : SecurityContext() {
  // RemoteSecurityContext's origin is expected to stay uninitialized until
  // we set it using replicated origin data from the browser process.
  DCHECK(!GetSecurityOrigin());

  // Start with a clean slate.
  SetContentSecurityPolicy(ContentSecurityPolicy::Create());

  // FIXME: Document::initSecurityContext has a few other things we may
  // eventually want here, such as enforcing a setting to
  // grantUniversalAccess().
}

RemoteSecurityContext* RemoteSecurityContext::Create() {
  return new RemoteSecurityContext();
}

void RemoteSecurityContext::Trace(blink::Visitor* visitor) {
  SecurityContext::Trace(visitor);
}

void RemoteSecurityContext::SetReplicatedOrigin(
    scoped_refptr<SecurityOrigin> origin) {
  DCHECK(origin);
  SetSecurityOrigin(std::move(origin));
  GetContentSecurityPolicy()->SetupSelf(*GetSecurityOrigin());
}

void RemoteSecurityContext::ResetReplicatedContentSecurityPolicy() {
  DCHECK(GetSecurityOrigin());
  SetContentSecurityPolicy(ContentSecurityPolicy::Create());
  GetContentSecurityPolicy()->SetupSelf(*GetSecurityOrigin());
}

void RemoteSecurityContext::ResetSandboxFlags() {
  sandbox_flags_ = kSandboxNone;
}

}  // namespace blink
