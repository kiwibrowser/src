// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_function.h"

#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

namespace blink {

v8::Local<v8::Function> ScriptFunction::BindToV8Function() {
#if DCHECK_IS_ON()
  DCHECK(!bind_to_v8_function_already_called_);
  bind_to_v8_function_already_called_ = true;
#endif

  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::Local<v8::External> wrapper = v8::External::New(isolate, this);
  script_state_->World().RegisterDOMObjectHolder(isolate, this, wrapper);
  return v8::Function::New(script_state_->GetContext(), CallCallback, wrapper,
                           0, v8::ConstructorBehavior::kThrow)
      .ToLocalChecked();
}

void ScriptFunction::CallCallback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  DCHECK(args.Data()->IsExternal());
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(args.GetIsolate(),
                                               "Blink_CallCallback");
  ScriptFunction* script_function = static_cast<ScriptFunction*>(
      v8::Local<v8::External>::Cast(args.Data())->Value());
  ScriptValue result = script_function->Call(
      ScriptValue(script_function->GetScriptState(), args[0]));
  V8SetReturnValue(args, result.V8Value());
}

}  // namespace blink
