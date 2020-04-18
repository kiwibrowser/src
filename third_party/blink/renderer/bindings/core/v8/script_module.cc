// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_module.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/script_module_resolver.h"

namespace blink {

ScriptModule::ScriptModule() = default;

ScriptModule::ScriptModule(v8::Isolate* isolate,
                           v8::Local<v8::Module> module,
                           const KURL& source_url)
    : module_(SharedPersistent<v8::Module>::Create(module, isolate)),
      identity_hash_(static_cast<unsigned>(module->GetIdentityHash())),
      source_url_(source_url.GetString()) {
  DCHECK(!module_->IsEmpty());
}

ScriptModule::~ScriptModule() = default;

ScriptModule ScriptModule::Compile(v8::Isolate* isolate,
                                   const String& source,
                                   const KURL& source_url,
                                   const KURL& base_url,
                                   const ScriptFetchOptions& options,
                                   AccessControlStatus access_control_status,
                                   const TextPosition& text_position,
                                   ExceptionState& exception_state) {
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Module> module;

  if (!V8ScriptRunner::CompileModule(isolate, source, source_url,
                                     access_control_status, text_position,
                                     ReferrerScriptInfo(base_url, options))
           .ToLocal(&module)) {
    DCHECK(try_catch.HasCaught());
    exception_state.RethrowV8Exception(try_catch.Exception());
    return ScriptModule();
  }
  DCHECK(!try_catch.HasCaught());
  return ScriptModule(isolate, module, source_url);
}

ScriptValue ScriptModule::Instantiate(ScriptState* script_state) {
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);

  DCHECK(!IsNull());
  v8::Local<v8::Context> context = script_state->GetContext();
  probe::ExecuteScript probe(ExecutionContext::From(script_state), source_url_);
  bool success;
  if (!NewLocal(script_state->GetIsolate())
           ->InstantiateModule(context, &ResolveModuleCallback)
           .To(&success) ||
      !success) {
    DCHECK(try_catch.HasCaught());
    return ScriptValue(script_state, try_catch.Exception());
  }
  DCHECK(!try_catch.HasCaught());
  return ScriptValue();
}

ScriptValue ScriptModule::Evaluate(ScriptState* script_state) const {
  v8::Isolate* isolate = script_state->GetIsolate();

  // Isolate exceptions that occur when executing the code. These exceptions
  // should not interfere with javascript code we might evaluate from C++ when
  // returning from here.
  v8::TryCatch try_catch(isolate);

  probe::ExecuteScript probe(ExecutionContext::From(script_state), source_url_);

  // TODO(kouhei): We currently don't have a code-path which use return value of
  // EvaluateModule. Stop ignoring result once we have such path.
  v8::Local<v8::Value> result;
  if (!V8ScriptRunner::EvaluateModule(isolate, module_->NewLocal(isolate),
                                      script_state->GetContext())
           .ToLocal(&result)) {
    DCHECK(try_catch.HasCaught());
    return ScriptValue(script_state, try_catch.Exception());
  }

  return ScriptValue();
}

void ScriptModule::ReportException(ScriptState* script_state,
                                   v8::Local<v8::Value> exception) {
  V8ScriptRunner::ReportException(script_state->GetIsolate(), exception);
}

Vector<String> ScriptModule::ModuleRequests(ScriptState* script_state) {
  if (IsNull())
    return Vector<String>();

  v8::Local<v8::Module> module = module_->NewLocal(script_state->GetIsolate());

  Vector<String> ret;

  int length = module->GetModuleRequestsLength();
  ret.ReserveInitialCapacity(length);
  for (int i = 0; i < length; ++i) {
    v8::Local<v8::String> v8_name = module->GetModuleRequest(i);
    ret.push_back(ToCoreString(v8_name));
  }
  return ret;
}

Vector<TextPosition> ScriptModule::ModuleRequestPositions(
    ScriptState* script_state) {
  if (IsNull())
    return Vector<TextPosition>();
  v8::Local<v8::Module> module = module_->NewLocal(script_state->GetIsolate());

  Vector<TextPosition> ret;

  int length = module->GetModuleRequestsLength();
  ret.ReserveInitialCapacity(length);
  for (int i = 0; i < length; ++i) {
    v8::Location v8_loc = module->GetModuleRequestLocation(i);
    ret.emplace_back(OrdinalNumber::FromZeroBasedInt(v8_loc.GetLineNumber()),
                     OrdinalNumber::FromZeroBasedInt(v8_loc.GetColumnNumber()));
  }
  return ret;
}

v8::Local<v8::Value> ScriptModule::V8Namespace(v8::Isolate* isolate) {
  DCHECK(!IsNull());
  v8::Local<v8::Module> module = module_->NewLocal(isolate);
  return module->GetModuleNamespace();
}

v8::MaybeLocal<v8::Module> ScriptModule::ResolveModuleCallback(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::Module> referrer) {
  v8::Isolate* isolate = context->GetIsolate();
  Modulator* modulator = Modulator::From(ScriptState::From(context));
  DCHECK(modulator);

  // TODO(shivanisha): Can a valid source url be passed to the constructor.
  ScriptModule referrer_record(isolate, referrer, KURL());
  ExceptionState exception_state(isolate, ExceptionState::kExecutionContext,
                                 "ScriptModule", "resolveModuleCallback");
  ScriptModule resolved = modulator->GetScriptModuleResolver()->Resolve(
      ToCoreStringWithNullCheck(specifier), referrer_record, exception_state);
  DCHECK(!resolved.IsNull());
  DCHECK(!exception_state.HadException());
  return resolved.module_->NewLocal(isolate);
}

}  // namespace blink
