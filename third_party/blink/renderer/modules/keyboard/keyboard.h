// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class ExecutionContext;
class KeyboardLayout;
class KeyboardLock;
class ScriptState;

class Keyboard final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(Keyboard);

 public:
  explicit Keyboard(ExecutionContext*);
  ~Keyboard() override;

  // KeyboardLock API: https://w3c.github.io/keyboard-lock/
  ScriptPromise lock(ScriptState*, const Vector<String>&);
  void unlock(ScriptState*);

  ScriptPromise getLayoutMap(ScriptState*);

  // ScriptWrappable override.
  void Trace(blink::Visitor*) override;

 private:
  Member<KeyboardLock> keyboard_lock_;
  Member<KeyboardLayout> keyboard_layout_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_KEYBOARD_H_
