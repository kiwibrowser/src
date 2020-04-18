// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_REMOTE_SECURITY_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_REMOTE_SECURITY_CONTEXT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/security_context.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CORE_EXPORT RemoteSecurityContext
    : public GarbageCollectedFinalized<RemoteSecurityContext>,
      public SecurityContext {
  USING_GARBAGE_COLLECTED_MIXIN(RemoteSecurityContext);

 public:
  void Trace(blink::Visitor*) override;

  static RemoteSecurityContext* Create();
  void SetReplicatedOrigin(scoped_refptr<SecurityOrigin>);
  void ResetReplicatedContentSecurityPolicy();
  void ResetSandboxFlags();

  // FIXME: implement
  void DidUpdateSecurityOrigin() override {}

 private:
  RemoteSecurityContext();
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_REMOTE_SECURITY_CONTEXT_H_
