// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/streams/underlying_source_base.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/streams/readable_stream_default_controller_wrapper.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "v8/include/v8.h"

namespace blink {

ScriptPromise UnderlyingSourceBase::startWrapper(ScriptState* script_state,
                                                 ScriptValue js_controller) {
  // Cannot call start twice (e.g., cannot use the same UnderlyingSourceBase to
  // construct multiple streams).
  DCHECK(!controller_);

  controller_ = new ReadableStreamDefaultControllerWrapper(js_controller);

  return Start(script_state);
}

ScriptPromise UnderlyingSourceBase::Start(ScriptState* script_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise UnderlyingSourceBase::pull(ScriptState* script_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise UnderlyingSourceBase::cancelWrapper(ScriptState* script_state,
                                                  ScriptValue reason) {
  if (controller_)
    controller_->NoteHasBeenCanceled();
  return Cancel(script_state, reason);
}

ScriptPromise UnderlyingSourceBase::Cancel(ScriptState* script_state,
                                           ScriptValue reason) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptValue UnderlyingSourceBase::type(ScriptState* script_state) const {
  return ScriptValue(script_state, v8::Undefined(script_state->GetIsolate()));
}

void UnderlyingSourceBase::notifyLockAcquired() {
  is_stream_locked_ = true;
}

void UnderlyingSourceBase::notifyLockReleased() {
  is_stream_locked_ = false;
}

bool UnderlyingSourceBase::HasPendingActivity() const {
  // This will return false within a finite time period _assuming_ that
  // consumers use the controller to close or error the stream.
  // Browser-created readable streams should always close or error within a
  // finite time period, due to timeouts etc.
  return controller_ && controller_->IsActive() && is_stream_locked_;
}

void UnderlyingSourceBase::ContextDestroyed(ExecutionContext*) {
  if (controller_) {
    controller_->NoteHasBeenCanceled();
    controller_.Clear();
  }
}

void UnderlyingSourceBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(controller_);
  ScriptWrappable::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
