// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/animator_definition.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/animationworklet/animator.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"
#include "third_party/blink/renderer/platform/bindings/v8_object_constructor.h"

namespace blink {

AnimatorDefinition::AnimatorDefinition(v8::Isolate* isolate,
                                       v8::Local<v8::Function> constructor,
                                       v8::Local<v8::Function> animate)
    : constructor_(isolate, constructor), animate_(isolate, animate) {}

AnimatorDefinition::~AnimatorDefinition() = default;

void AnimatorDefinition::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(constructor_.Cast<v8::Value>());
  visitor->TraceWrappers(animate_.Cast<v8::Value>());
}

v8::Local<v8::Function> AnimatorDefinition::ConstructorLocal(
    v8::Isolate* isolate) {
  return constructor_.NewLocal(isolate);
}

v8::Local<v8::Function> AnimatorDefinition::AnimateLocal(v8::Isolate* isolate) {
  return animate_.NewLocal(isolate);
}

}  // namespace blink
