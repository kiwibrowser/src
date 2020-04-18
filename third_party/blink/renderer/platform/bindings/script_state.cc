// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/script_state.h"

#include "third_party/blink/renderer/platform/bindings/v8_binding.h"
#include "third_party/blink/renderer/platform/instance_counters.h"

namespace blink {

scoped_refptr<ScriptState> ScriptState::Create(
    v8::Local<v8::Context> context,
    scoped_refptr<DOMWrapperWorld> world) {
  scoped_refptr<ScriptState> script_state =
      base::AdoptRef(new ScriptState(context, std::move(world)));
  // This AddRef() is for keeping this ScriptState alive as long as the
  // v8::Context is alive.  This is Release()d in the weak callback of the
  // v8::Context.
  script_state->AddRef();
  return script_state;
}

static void DerefCallback(const v8::WeakCallbackInfo<ScriptState>& data) {
  data.GetParameter()->Release();
}

static void ContextCollectedCallback(
    const v8::WeakCallbackInfo<ScriptState>& data) {
  data.GetParameter()->ClearContext();
  data.SetSecondPassCallback(DerefCallback);
}

ScriptState::ScriptState(v8::Local<v8::Context> context,
                         scoped_refptr<DOMWrapperWorld> world)
    : isolate_(context->GetIsolate()),
      context_(isolate_, context),
      world_(std::move(world)),
      per_context_data_(V8PerContextData::Create(context)) {
  DCHECK(world_);
  context_.SetWeak(this, &ContextCollectedCallback);
  context->SetAlignedPointerInEmbedderData(kV8ContextPerContextDataIndex, this);
}

ScriptState::~ScriptState() {
  DCHECK(!per_context_data_);
  DCHECK(context_.IsEmpty());
  InstanceCounters::DecrementCounter(
      InstanceCounters::kDetachedScriptStateCounter);
}

void ScriptState::DetachGlobalObject() {
  DCHECK(!context_.IsEmpty());
  GetContext()->DetachGlobal();
}

void ScriptState::DisposePerContextData() {
  per_context_data_ = nullptr;
  InstanceCounters::IncrementCounter(
      InstanceCounters::kDetachedScriptStateCounter);
}

}  // namespace blink
