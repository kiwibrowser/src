// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_PERMISSIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_PERMISSIONS_H_

#include "third_party/blink/public/platform/modules/permissions/permission.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Dictionary;
class ExecutionContext;
class ScriptPromiseResolver;
class ScriptState;

class Permissions final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ScriptPromise query(ScriptState*, const Dictionary&);
  ScriptPromise request(ScriptState*, const Dictionary&);
  ScriptPromise revoke(ScriptState*, const Dictionary&);
  ScriptPromise requestAll(ScriptState*, const Vector<Dictionary>&);

 private:
  mojom::blink::PermissionService& GetService(ExecutionContext*);
  void ServiceConnectionError();
  void TaskComplete(ScriptPromiseResolver*,
                    mojom::blink::PermissionDescriptorPtr,
                    mojom::blink::PermissionStatus);
  void BatchTaskComplete(ScriptPromiseResolver*,
                         Vector<mojom::blink::PermissionDescriptorPtr>,
                         Vector<int>,
                         const Vector<mojom::blink::PermissionStatus>&);

  mojom::blink::PermissionServicePtr service_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_PERMISSIONS_H_
