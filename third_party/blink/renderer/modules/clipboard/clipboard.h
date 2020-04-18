// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class DataTransfer;
class ScriptState;

class Clipboard : public EventTargetWithInlineData,
                  public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(Clipboard);
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(Clipboard);

 public:
  explicit Clipboard(ExecutionContext*);

  ScriptPromise read(ScriptState*);
  ScriptPromise readText(ScriptState*);

  ScriptPromise write(ScriptState*, DataTransfer*);
  ScriptPromise writeText(ScriptState*, const String&);

  // EventTarget
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  void Trace(blink::Visitor*) override;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_CLIPBOARD_H_
