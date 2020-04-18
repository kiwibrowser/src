/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/bindings/core/v8/v8_abstract_event_listener.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_event.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_event_listener_helper.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_event_target.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_parser.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/before_unload_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/instance_counters.h"

namespace blink {

V8AbstractEventListener::V8AbstractEventListener(v8::Isolate* isolate,
                                                 bool is_attribute,
                                                 DOMWrapperWorld& world)
    : EventListener(kJSEventListenerType),
      is_attribute_(is_attribute),
      world_(&world),
      isolate_(isolate),
      worker_or_worklet_global_scope_(nullptr) {
  if (IsMainThread())
    InstanceCounters::IncrementCounter(
        InstanceCounters::kJSEventListenerCounter);
  else
    worker_or_worklet_global_scope_ =
        ToWorkerOrWorkletGlobalScope(CurrentExecutionContext(isolate));
}

V8AbstractEventListener::~V8AbstractEventListener() {
  DCHECK(listener_.IsEmpty());
  if (IsMainThread())
    InstanceCounters::DecrementCounter(
        InstanceCounters::kJSEventListenerCounter);
}

void V8AbstractEventListener::handleEvent(ExecutionContext* execution_context,
                                          Event* event) {
  if (!execution_context)
    return;
  // Don't reenter V8 if execution was terminated in this instance of V8.
  if (execution_context->IsJSExecutionForbidden())
    return;

  // A ScriptState used by the event listener needs to be calculated based on
  // the ExecutionContext that fired the the event listener and the world
  // that installed the event listener.
  DCHECK(event);
  v8::HandleScope handle_scope(ToIsolate(execution_context));
  v8::Local<v8::Context> v8_context = ToV8Context(execution_context, World());
  if (v8_context.IsEmpty())
    return;
  ScriptState* script_state = ScriptState::From(v8_context);
  if (!script_state->ContextIsValid())
    return;
  HandleEvent(script_state, event);
}

void V8AbstractEventListener::HandleEvent(ScriptState* script_state,
                                          Event* event) {
  ScriptState::Scope scope(script_state);

  // Get the V8 wrapper for the event object.
  v8::Local<v8::Value> js_event =
      ToV8(event, script_state->GetContext()->Global(), GetIsolate());
  if (js_event.IsEmpty())
    return;
  InvokeEventHandler(script_state, event,
                     v8::Local<v8::Value>::New(GetIsolate(), js_event));
}

void V8AbstractEventListener::SetListenerObject(
    v8::Local<v8::Object> listener) {
  DCHECK(listener_.IsEmpty());
  // Balanced in WrapperCleared xor ClearListenerObject.
  if (worker_or_worklet_global_scope_) {
    worker_or_worklet_global_scope_->RegisterEventListener(this);
  } else {
    keep_alive_ = this;
  }
  listener_.Set(GetIsolate(), listener, this, &WrapperCleared);
}

void V8AbstractEventListener::InvokeEventHandler(
    ScriptState* script_state,
    Event* event,
    v8::Local<v8::Value> js_event) {
  if (!event->CanBeDispatchedInWorld(World()))
    return;

  v8::Local<v8::Value> return_value;
  v8::Local<v8::Context> context = script_state->GetContext();
  {
    // Catch exceptions thrown in the event handler so they do not propagate to
    // javascript code that caused the event to fire.
    v8::TryCatch try_catch(GetIsolate());
    try_catch.SetVerbose(true);

    v8::Local<v8::Object> global = context->Global();
    V8PrivateProperty::Symbol event_symbol =
        V8PrivateProperty::GetGlobalEvent(GetIsolate());
    // Save the old 'event' property so we can restore it later.
    v8::Local<v8::Value> saved_event;
    if (!event_symbol.GetOrUndefined(global).ToLocal(&saved_event))
      return;
    try_catch.Reset();

    // Expose the event object as |window.event|, except when the event's target
    // is in a V1 shadow tree, in which case |window.event| should be
    // |undefined|.
    Node* target_node = event->target()->ToNode();
    if (target_node && target_node->IsInV1ShadowTree()) {
      event_symbol.Set(global, v8::Undefined(GetIsolate()));
    } else {
      event_symbol.Set(global, js_event);
    }
    try_catch.Reset();

    return_value = CallListenerFunction(script_state, js_event, event);
    if (try_catch.HasCaught())
      event->target()->UncaughtExceptionInEventHandler();

    if (!try_catch.CanContinue()) {  // Result of TerminateExecution().
      ExecutionContext* execution_context = ToExecutionContext(context);
      if (execution_context->IsWorkerOrWorkletGlobalScope())
        ToWorkerOrWorkletGlobalScope(execution_context)
            ->ScriptController()
            ->ForbidExecution();
      return;
    }
    try_catch.Reset();

    // Restore the old event. This must be done for all exit paths through this
    // method.
    event_symbol.Set(global, saved_event);
    try_catch.Reset();
  }

  if (return_value.IsEmpty())
    return;

  // Because OnBeforeUnloadEventHandler is currently not implemented, the
  // following special handling of BeforeUnloadEvents and events with the type
  // beforeunload is needed in accordance with the spec at
  // https://html.spec.whatwg.org/multipage/webappapis.html#the-event-handler-processing-algorithm
  // TODO(rakina): remove special handling after OnBeforeUnloadEventHandler
  // is implemented

  if (!is_attribute_) {
    return;
  }

  if (event->IsBeforeUnloadEvent() &&
      event->type() == EventTypeNames::beforeunload) {
    if (!return_value->IsUndefined() && !return_value->IsNull()) {
      event->preventDefault();
      if (ToBeforeUnloadEvent(event)->returnValue().IsEmpty()) {
        TOSTRING_VOID(V8StringResource<>, string_return_value, return_value);
        ToBeforeUnloadEvent(event)->setReturnValue(string_return_value);
      }
    }
  } else if (ShouldPreventDefault(return_value) &&
             event->type() != EventTypeNames::beforeunload) {
    event->preventDefault();
  }
}

bool V8AbstractEventListener::ShouldPreventDefault(
    v8::Local<v8::Value> return_value) {
  // Prevent default action if the return value is false in accord with the spec
  // http://www.w3.org/TR/html5/webappapis.html#event-handler-attributes
  return return_value->IsBoolean() && !return_value.As<v8::Boolean>()->Value();
}

v8::Local<v8::Object> V8AbstractEventListener::GetReceiverObject(
    ScriptState* script_state,
    Event* event) {
  v8::Local<v8::Object> listener = listener_.NewLocal(GetIsolate());
  if (!listener_.IsEmpty() && !listener->IsFunction())
    return listener;

  EventTarget* target = event->currentTarget();
  v8::Local<v8::Value> value =
      ToV8(target, script_state->GetContext()->Global(), GetIsolate());
  if (value.IsEmpty())
    return v8::Local<v8::Object>();
  return v8::Local<v8::Object>::New(GetIsolate(),
                                    v8::Local<v8::Object>::Cast(value));
}

bool V8AbstractEventListener::BelongsToTheCurrentWorld(
    ExecutionContext* execution_context) const {
  if (!GetIsolate()->GetCurrentContext().IsEmpty() &&
      &World() == &DOMWrapperWorld::Current(GetIsolate()))
    return true;
  // If currently parsing, the parser could be accessing this listener
  // outside of any v8 context; check if it belongs to the main world.
  if (!GetIsolate()->InContext() && execution_context &&
      execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    if (document->Parser() && document->Parser()->IsParsing())
      return World().IsMainWorld();
  }
  return false;
}

void V8AbstractEventListener::ClearListenerObject() {
  if (!HasExistingListenerObject())
    return;
  probe::AsyncTaskCanceled(GetIsolate(), this);
  listener_.Clear();
  if (worker_or_worklet_global_scope_) {
    worker_or_worklet_global_scope_->DeregisterEventListener(this);
  } else {
    keep_alive_.Clear();
  }
}

void V8AbstractEventListener::WrapperCleared(
    const v8::WeakCallbackInfo<V8AbstractEventListener>& data) {
  data.GetParameter()->ClearListenerObject();
}

void V8AbstractEventListener::Trace(blink::Visitor* visitor) {
  visitor->Trace(worker_or_worklet_global_scope_);
  EventListener::Trace(visitor);
}

void V8AbstractEventListener::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(listener_.Cast<v8::Value>());
  EventListener::TraceWrappers(visitor);
}

}  // namespace blink
