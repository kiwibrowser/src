// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_intersection_observer_delegate.h"

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_intersection_observer_callback.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

V8IntersectionObserverDelegate::V8IntersectionObserverDelegate(
    V8IntersectionObserverCallback* callback,
    ScriptState* script_state)
    : ContextClient(ExecutionContext::From(script_state)),
      callback_(callback) {}

V8IntersectionObserverDelegate::~V8IntersectionObserverDelegate() = default;

void V8IntersectionObserverDelegate::Deliver(
    const HeapVector<Member<IntersectionObserverEntry>>& entries,
    IntersectionObserver& observer) {
  callback_->InvokeAndReportException(&observer, entries, &observer);
}

ExecutionContext* V8IntersectionObserverDelegate::GetExecutionContext() const {
  return ContextClient::GetExecutionContext();
}

void V8IntersectionObserverDelegate::Trace(blink::Visitor* visitor) {
  visitor->Trace(callback_);
  IntersectionObserverDelegate::Trace(visitor);
  ContextClient::Trace(visitor);
}

void V8IntersectionObserverDelegate::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(callback_);
  IntersectionObserverDelegate::TraceWrappers(visitor);
}

}  // namespace blink
