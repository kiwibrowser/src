// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LAYOUT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LAYOUT_H_

#include "third_party/blink/public/platform/modules/keyboard_lock/keyboard_lock.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/keyboard/keyboard_layout_map.h"

namespace blink {

class ScriptPromiseResolver;

class KeyboardLayout final : public GarbageCollectedFinalized<KeyboardLayout>,
                             public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(KeyboardLayout);
  WTF_MAKE_NONCOPYABLE(KeyboardLayout);

 public:
  explicit KeyboardLayout(ExecutionContext*);
  virtual ~KeyboardLayout() = default;

  ScriptPromise GetKeyboardLayoutMap(ScriptState*);

  // ContextLifecycleObserver override.
  void Trace(blink::Visitor*) override;

 private:
  bool EnsureServiceConnected();

  void GotKeyboardLayoutMap(ScriptPromiseResolver*,
                            mojom::blink::GetKeyboardLayoutMapResultPtr);

  Member<ScriptPromiseResolver> script_promise_resolver_;

  mojom::blink::KeyboardLockServicePtr service_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_LAYOUT_H_
