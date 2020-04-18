// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_PROMISE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_PROMISE_H_

#include "third_party/blink/public/mojom/clipboard/clipboard.mojom-blink.h"
#include "third_party/blink/public/platform/modules/permissions/permission.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"

namespace blink {

class DataTransfer;
class ScriptPromiseResolver;

class ClipboardPromise final
    : public GarbageCollectedFinalized<ClipboardPromise>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(ClipboardPromise);
  WTF_MAKE_NONCOPYABLE(ClipboardPromise);

 public:
  virtual ~ClipboardPromise() = default;

  static ScriptPromise CreateForRead(ScriptState*);
  static ScriptPromise CreateForReadText(ScriptState*);
  static ScriptPromise CreateForWrite(ScriptState*, DataTransfer*);
  static ScriptPromise CreateForWriteText(ScriptState*, const String&);

  void Trace(blink::Visitor*) override;

 private:
  ClipboardPromise(ScriptState*);

  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner();
  mojom::blink::PermissionService* GetPermissionService();

  bool IsFocusedDocument(ExecutionContext*);

  void RequestReadPermission(
      mojom::blink::PermissionService::RequestPermissionCallback);
  void CheckWritePermission(
      mojom::blink::PermissionService::HasPermissionCallback);

  void HandleRead();
  void HandleReadWithPermission(mojom::blink::PermissionStatus);

  void HandleReadText();
  void HandleReadTextWithPermission(mojom::blink::PermissionStatus);

  void HandleWrite(DataTransfer*);
  void HandleWriteWithPermission(mojom::blink::PermissionStatus);

  void HandleWriteText(const String&);
  void HandleWriteTextWithPermission(mojom::blink::PermissionStatus);

  ScriptState* script_state_;

  Member<ScriptPromiseResolver> script_promise_resolver_;

  mojom::blink::PermissionServicePtr permission_service_;

  mojom::ClipboardBuffer buffer_;

  String write_data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_PROMISE_H_
