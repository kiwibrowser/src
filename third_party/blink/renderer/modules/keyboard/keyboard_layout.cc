// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/keyboard/keyboard_layout.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

using mojom::PageVisibilityState;

KeyboardLayout::KeyboardLayout(ExecutionContext* context)
    : ContextLifecycleObserver(context) {}

ScriptPromise KeyboardLayout::GetKeyboardLayoutMap(ScriptState* script_state) {
  DCHECK(script_state);

  script_promise_resolver_ = ScriptPromiseResolver::Create(script_state);
  if (EnsureServiceConnected()) {
    service_->GetKeyboardLayoutMap(
        WTF::Bind(&KeyboardLayout::GotKeyboardLayoutMap, WrapPersistent(this),
                  WrapPersistent(script_promise_resolver_.Get())));
    return script_promise_resolver_->Promise();
  }

  script_promise_resolver_->Reject();
  return script_promise_resolver_->Promise();
}

bool KeyboardLayout::EnsureServiceConnected() {
  if (!service_) {
    LocalFrame* frame = GetFrame();
    if (!frame) {
      return false;
    }
    frame->GetInterfaceProvider().GetInterface(mojo::MakeRequest(&service_));
  }
  DCHECK(service_);
  return true;
}

void KeyboardLayout::GotKeyboardLayoutMap(
    ScriptPromiseResolver* resolver,
    mojom::blink::GetKeyboardLayoutMapResultPtr result) {
  DCHECK(script_promise_resolver_);

  if (result->status == mojom::blink::GetKeyboardLayoutMapStatus::kSuccess) {
    script_promise_resolver_->Resolve(
        new KeyboardLayoutMap(result->layout_map));
    return;
  }

  script_promise_resolver_->Reject();
}

void KeyboardLayout::Trace(blink::Visitor* visitor) {
  visitor->Trace(script_promise_resolver_);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
