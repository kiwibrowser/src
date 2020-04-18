// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LOCK_H_

#include "third_party/blink/public/platform/modules/keyboard_lock/keyboard_lock.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

class ScriptPromiseResolver;

class KeyboardLock final : public GarbageCollectedFinalized<KeyboardLock>,
                           public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(KeyboardLock);
  WTF_MAKE_NONCOPYABLE(KeyboardLock);

 public:
  explicit KeyboardLock(ExecutionContext*);
  ~KeyboardLock();

  ScriptPromise lock(ScriptState*, const Vector<String>&);
  void unlock(ScriptState*);

  // ContextLifecycleObserver override.
  void Trace(blink::Visitor*) override;

 private:
  // Returns true if |service_| is initialized and ready to be called.
  bool EnsureServiceConnected();

  // Returns true if the current frame is a top-level browsing context.
  bool CalledFromSupportedContext(ExecutionContext* context);

  void LockRequestFinished(ScriptPromiseResolver*,
                           mojom::KeyboardLockRequestResult);

  mojom::blink::KeyboardLockServicePtr service_;
  Member<ScriptPromiseResolver> request_keylock_resolver_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LOCK_H_
