// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/animator.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/animationworklet/animator_definition.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/to_v8.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

namespace blink {

Animator::Animator(v8::Isolate* isolate,
                   AnimatorDefinition* definition,
                   v8::Local<v8::Object> instance)
    : definition_(definition),
      instance_(isolate, instance),
      effect_(new EffectProxy()) {}

Animator::~Animator() = default;

void Animator::Trace(blink::Visitor* visitor) {
  visitor->Trace(definition_);
  visitor->Trace(effect_);
}

void Animator::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(definition_);
  visitor->TraceWrappers(instance_.Cast<v8::Value>());
}

bool Animator::Animate(ScriptState* script_state,
                       const CompositorMutatorInputState::AnimationState& input,
                       CompositorMutatorOutputState::AnimationState* output) {
  did_animate_ = true;

  v8::Isolate* isolate = script_state->GetIsolate();

  v8::Local<v8::Object> instance = instance_.NewLocal(isolate);
  v8::Local<v8::Function> animate = definition_->AnimateLocal(isolate);

  if (IsUndefinedOrNull(instance) || IsUndefinedOrNull(animate))
    return false;

  ScriptState::Scope scope(script_state);
  v8::TryCatch block(isolate);
  block.SetVerbose(true);

  // Prepare arguments (i.e., current time and effect) and pass them to animate
  // callback.
  v8::Local<v8::Value> v8_effect =
      ToV8(effect_, script_state->GetContext()->Global(), isolate);

  v8::Local<v8::Value> v8_current_time =
      ToV8(input.current_time, script_state->GetContext()->Global(), isolate);

  v8::Local<v8::Value> argv[] = {v8_current_time, v8_effect};

  V8ScriptRunner::CallFunction(animate, ExecutionContext::From(script_state),
                               instance, arraysize(argv), argv, isolate);

  // The animate function may have produced an error!
  // TODO(majidvp): We should probably just throw here.
  if (block.HasCaught())
    return false;

  output->local_time = effect_->GetLocalTime();
  return true;
}

}  // namespace blink
