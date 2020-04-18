// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/callback_interface.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off

#include "third_party/blink/renderer/bindings/tests/results/core/v8_test_callback_interface.h"

#include "third_party/blink/renderer/bindings/core/v8/generated_code_helper.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_test_interface_empty.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {

v8::Maybe<void> V8TestCallbackInterface::voidMethod(ScriptWrappable* callback_this_value) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethod"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethod",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Value> *argv = nullptr;

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          0,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<bool> V8TestCallbackInterface::booleanMethod(ScriptWrappable* callback_this_value) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "booleanMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<bool>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "booleanMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<bool>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "booleanMethod"))
        .ToLocal(&value)) {
      return v8::Nothing<bool>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "booleanMethod",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<bool>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Value> *argv = nullptr;

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          0,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<bool>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  {
    ExceptionState exception_state(GetIsolate(),
                                   ExceptionState::kExecutionContext,
                                   "TestCallbackInterface",
                                   "booleanMethod");
    auto native_result =
        NativeValueTraits<IDLBoolean>::NativeValue(
            GetIsolate(), call_result, exception_state);
    if (exception_state.HadException())
      return v8::Nothing<bool>();
    else
      return v8::Just<bool>(native_result);
  }
}

v8::Maybe<void> V8TestCallbackInterface::voidMethodBooleanArg(ScriptWrappable* callback_this_value, bool boolArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodBooleanArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodBooleanArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethodBooleanArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodBooleanArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> boolArgHandle = v8::Boolean::New(GetIsolate(), boolArg);
  v8::Local<v8::Value> argv[] = { boolArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::voidMethodSequenceArg(ScriptWrappable* callback_this_value, const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodSequenceArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodSequenceArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethodSequenceArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodSequenceArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> sequenceArgHandle = ToV8(sequenceArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { sequenceArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::voidMethodFloatArg(ScriptWrappable* callback_this_value, float floatArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodFloatArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodFloatArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethodFloatArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodFloatArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> floatArgHandle = v8::Number::New(GetIsolate(), floatArg);
  v8::Local<v8::Value> argv[] = { floatArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::voidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethodTestInterfaceEmptyArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodTestInterfaceEmptyArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::voidMethodTestInterfaceEmptyStringArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "voidMethodTestInterfaceEmptyStringArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodTestInterfaceEmptyStringArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle, stringArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          2,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::callbackWithThisValueVoidMethodStringArg(ScriptWrappable* callback_this_value, const String& stringArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "callbackWithThisValueVoidMethodStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "callbackWithThisValueVoidMethodStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "callbackWithThisValueVoidMethodStringArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "callbackWithThisValueVoidMethodStringArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { stringArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

v8::Maybe<void> V8TestCallbackInterface::customVoidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  if (!IsCallbackFunctionRunnable(CallbackRelevantScriptState())) {
    // Wrapper-tracing for the callback function makes the function object and
    // its creation context alive. Thus it's safe to use the creation context
    // of the callback function here.
    v8::HandleScope handle_scope(GetIsolate());
    CHECK(!CallbackObject().IsEmpty());
    v8::Context::Scope context_scope(CallbackObject()->CreationContext());
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "customVoidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(
      CallbackRelevantScriptState());
  // step 8. Prepare to run a callback with stored settings.
  if (IncumbentScriptState()->GetContext().IsEmpty()) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "customVoidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<void>();
  }
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentScriptState()->GetContext());

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantScriptState()->GetContext(),
                               V8String(GetIsolate(), "customVoidMethodTestInterfaceEmptyArg"))
        .ToLocal(&value)) {
      return v8::Nothing<void>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "customVoidMethodTestInterfaceEmptyArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<void>();
    }
    function = value.As<v8::Function>();
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantScriptState());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantScriptState()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::Local<v8::Value> call_result;
  if (!V8ScriptRunner::CallFunction(
          function,
          ExecutionContext::From(CallbackRelevantScriptState()),
          this_arg,
          1,
          argv,
          GetIsolate()).ToLocal(&call_result)) {
    // step 14. If callResult is an abrupt completion, set completion to
    //   callResult and jump to the step labeled return.
    return v8::Nothing<void>();
  }

  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return v8::JustVoid();
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethod(ScriptWrappable* callback_this_value) {
  return Proxy()->voidMethod(
      callback_this_value);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<bool> V8PersistentCallbackInterface<V8TestCallbackInterface>::booleanMethod(ScriptWrappable* callback_this_value) {
  return Proxy()->booleanMethod(
      callback_this_value);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethodBooleanArg(ScriptWrappable* callback_this_value, bool boolArg) {
  return Proxy()->voidMethodBooleanArg(
      callback_this_value, boolArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethodSequenceArg(ScriptWrappable* callback_this_value, const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) {
  return Proxy()->voidMethodSequenceArg(
      callback_this_value, sequenceArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethodFloatArg(ScriptWrappable* callback_this_value, float floatArg) {
  return Proxy()->voidMethodFloatArg(
      callback_this_value, floatArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) {
  return Proxy()->voidMethodTestInterfaceEmptyArg(
      callback_this_value, testInterfaceEmptyArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::voidMethodTestInterfaceEmptyStringArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) {
  return Proxy()->voidMethodTestInterfaceEmptyStringArg(
      callback_this_value, testInterfaceEmptyArg, stringArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::callbackWithThisValueVoidMethodStringArg(ScriptWrappable* callback_this_value, const String& stringArg) {
  return Proxy()->callbackWithThisValueVoidMethodStringArg(
      callback_this_value, stringArg);
}

CORE_EXTERN_TEMPLATE_EXPORT
v8::Maybe<void> V8PersistentCallbackInterface<V8TestCallbackInterface>::customVoidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) {
  return Proxy()->customVoidMethodTestInterfaceEmptyArg(
      callback_this_value, testInterfaceEmptyArg);
}

}  // namespace blink
