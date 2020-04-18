// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/modulator_impl_base.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetch_request.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_loader_registry.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_tree_linker_registry.h"
#include "third_party/blink/renderer/core/script/dynamic_module_resolver.h"
#include "third_party/blink/renderer/core/script/module_map.h"
#include "third_party/blink/renderer/core/script/module_script.h"
#include "third_party/blink/renderer/core/script/script_module_resolver_impl.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"

namespace blink {

ExecutionContext* ModulatorImplBase::GetExecutionContext() const {
  return ExecutionContext::From(script_state_.get());
}

ModulatorImplBase::ModulatorImplBase(scoped_refptr<ScriptState> script_state)
    : script_state_(std::move(script_state)),
      task_runner_(ExecutionContext::From(script_state_.get())
                       ->GetTaskRunner(TaskType::kNetworking)),
      map_(ModuleMap::Create(this)),
      loader_registry_(ModuleScriptLoaderRegistry::Create()),
      tree_linker_registry_(ModuleTreeLinkerRegistry::Create()),
      script_module_resolver_(ScriptModuleResolverImpl::Create(
          this,
          ExecutionContext::From(script_state_.get()))),
      dynamic_module_resolver_(DynamicModuleResolver::Create(this)) {
  DCHECK(script_state_);
  DCHECK(task_runner_);
}

ModulatorImplBase::~ModulatorImplBase() {}

ReferrerPolicy ModulatorImplBase::GetReferrerPolicy() {
  return GetExecutionContext()->GetReferrerPolicy();
}

const SecurityOrigin* ModulatorImplBase::GetSecurityOriginForFetch() {
  return GetExecutionContext()->GetSecurityOrigin();
}

// [fetch-a-module-script-tree]
// https://html.spec.whatwg.org/multipage/webappapis.html#fetch-a-module-script-tree
// [fetch-a-module-worker-script-tree]
// https://html.spec.whatwg.org/multipage/webappapis.html#fetch-a-module-worker-script-tree
void ModulatorImplBase::FetchTree(const KURL& url,
                                  WebURLRequest::RequestContext destination,
                                  const ScriptFetchOptions& options,
                                  ModuleTreeClient* client) {
  // <spec label="fetch-a-module-script-tree" step="2">Perform the internal
  // module script graph fetching procedure given url, settings object,
  // destination, options, settings object, visited set, "client", and with the
  // top-level module fetch flag set. If the caller of this algorithm specified
  // custom perform the fetch steps, pass those along as well.</spec>

  // <spec label="fetch-a-module-worker-script-tree" step="3">Perform the
  // internal module script graph fetching procedure given url, fetch client
  // settings object, destination, options, module map settings object, visited
  // set, "client", and with the top-level module fetch flag set. If the caller
  // of this algorithm specified custom perform the fetch steps, pass those
  // along as well.</spec>

  tree_linker_registry_->Fetch(url, GetExecutionContext()->BaseURL(),
                               destination, options, this, client);

  // <spec label="fetch-a-module-script-tree" step="3">When the internal module
  // script graph fetching procedure asynchronously completes with result,
  // asynchronously complete this algorithm with result.</spec>

  // <spec label="fetch-a-module-worker-script-tree" step="4">When the internal
  // module script graph fetching procedure asynchronously completes with
  // result, asynchronously complete this algorithm with result.</spec>

  // Note: We delegate to ModuleTreeLinker to notify ModuleTreeClient.
}

void ModulatorImplBase::FetchDescendantsForInlineScript(
    ModuleScript* module_script,
    WebURLRequest::RequestContext destination,
    ModuleTreeClient* client) {
  tree_linker_registry_->FetchDescendantsForInlineScript(
      module_script, destination, this, client);
}

void ModulatorImplBase::FetchSingle(const ModuleScriptFetchRequest& request,
                                    ModuleGraphLevel level,
                                    SingleModuleClient* client) {
  map_->FetchSingleModuleScript(request, level, client);
}

void ModulatorImplBase::FetchNewSingleModule(
    const ModuleScriptFetchRequest& request,
    ModuleGraphLevel level,
    ModuleScriptLoaderClient* client) {
  loader_registry_->Fetch(request, level, this, client);
}

ModuleScript* ModulatorImplBase::GetFetchedModuleScript(const KURL& url) {
  return map_->GetFetchedModuleScript(url);
}

bool ModulatorImplBase::HasValidContext() {
  return script_state_->ContextIsValid();
}

void ModulatorImplBase::ResolveDynamically(
    const String& specifier,
    const KURL& referrer_url,
    const ReferrerScriptInfo& referrer_info,
    ScriptPromiseResolver* resolver) {
  String reason;
  if (IsDynamicImportForbidden(&reason)) {
    resolver->Reject(V8ThrowException::CreateTypeError(
        GetScriptState()->GetIsolate(), reason));
    return;
  }
  dynamic_module_resolver_->ResolveDynamically(specifier, referrer_url,
                                               referrer_info, resolver);
}

// https://html.spec.whatwg.org/multipage/webappapis.html#hostgetimportmetaproperties
ModuleImportMeta ModulatorImplBase::HostGetImportMetaProperties(
    ScriptModule record) const {
  // 1. Let module script be moduleRecord.[[HostDefined]]. [spec text]
  ModuleScript* module_script = script_module_resolver_->GetHostDefined(record);
  DCHECK(module_script);

  // 2. Let urlString be module script's base URL, serialized. [spec text]
  String url_string = module_script->BaseURL().GetString();

  // 3. Return <<Record { [[Key]]: "url", [[Value]]: urlString }>>. [spec text]
  return ModuleImportMeta(url_string);
}

ScriptModule ModulatorImplBase::CompileModule(
    const String& provided_source,
    const KURL& source_url,
    const KURL& base_url,
    const ScriptFetchOptions& options,
    AccessControlStatus access_control_status,
    const TextPosition& position,
    ExceptionState& exception_state) {
  // Implements Steps 3-5 of
  // https://html.spec.whatwg.org/multipage/webappapis.html#creating-a-module-script

  // Step 3. Let realm be the provided environment settings object's Realm.
  // Note: Realm is v8::Context.

  // Step 4. If scripting is disabled for the given environment settings
  // object's responsible browsing context, then let script source be the empty
  // string. Otherwise, let script source be the provided script source.
  String script_source;
  if (GetExecutionContext()->CanExecuteScripts(kAboutToExecuteScript))
    script_source = provided_source;

  // Step 5. Let result be ParseModule(script source, realm, script).
  ScriptState::Scope scope(script_state_.get());
  return ScriptModule::Compile(
      script_state_->GetIsolate(), script_source, source_url, base_url, options,
      access_control_status, position, exception_state);
}

ScriptValue ModulatorImplBase::InstantiateModule(ScriptModule script_module) {
  ScriptState::Scope scope(script_state_.get());
  return script_module.Instantiate(script_state_.get());
}

Vector<Modulator::ModuleRequest>
ModulatorImplBase::ModuleRequestsFromScriptModule(ScriptModule script_module) {
  ScriptState::Scope scope(script_state_.get());
  Vector<String> specifiers = script_module.ModuleRequests(script_state_.get());
  Vector<TextPosition> positions =
      script_module.ModuleRequestPositions(script_state_.get());
  DCHECK_EQ(specifiers.size(), positions.size());
  Vector<ModuleRequest> requests;
  requests.ReserveInitialCapacity(specifiers.size());
  for (size_t i = 0; i < specifiers.size(); ++i) {
    requests.emplace_back(specifiers[i], positions[i]);
  }
  return requests;
}

ScriptValue ModulatorImplBase::ExecuteModule(
    const ModuleScript* module_script,
    CaptureEvalErrorFlag capture_error) {
  // https://html.spec.whatwg.org/multipage/webappapis.html#run-a-module-script

  // Step 1. "If rethrow errors is not given, let it be false." [spec text]

  // Step 2. "Let settings be the settings object of script." [spec text]
  // The settings object is |this|.

  // Step 3. "Check if we can run script with settings.
  //          If this returns "do not run" then return." [spec text]
  if (!GetExecutionContext()->CanExecuteScripts(kAboutToExecuteScript))
    return ScriptValue();

  // Step 4. "Prepare to run script given settings." [spec text]
  // This is placed here to also cover ScriptModule::ReportException().
  ScriptState::Scope scope(script_state_.get());

  // Step 5. "Let evaluationStatus be null." [spec text]
  // |error| corresponds to "evaluationStatus of [[Type]]: throw".
  ScriptValue error;

  // Step 6. "If script's error to rethrow is not null," [spec text]
  if (module_script->HasErrorToRethrow()) {
    // Step 6. "then set evaluationStatus to Completion { [[Type]]: throw,
    // [[Value]]: script's error to rethrow, [[Target]]: empty }." [spec text]
    error = module_script->CreateErrorToRethrow();
  } else {
    // Step 7. "Otherwise:

    // Step 7.1. "Let record be script's record. [spec text]
    const ScriptModule& record = module_script->Record();
    CHECK(!record.IsNull());

    // Step 7.2. "Set evaluationStatus to record.Evaluate()." [spec text]
    error = record.Evaluate(script_state_.get());

    // "If Evaluate fails to complete as a result of the user agent aborting the
    // running script, then set evaluationStatus to Completion { [[Type]]:
    // throw, [[Value]]: a new "QuotaExceededError" DOMException, [[Target]]:
    // empty }." [spec text]
  }

  // Step 8. "If evaluationStatus is an abrupt completion, then:" [spec text]
  if (!error.IsEmpty()) {
    // Step 8.1. "If rethrow errors is true, rethrow the exception given by
    // evaluationStatus.[[Value]]." [spec text]
    if (capture_error == CaptureEvalErrorFlag::kCapture)
      return error;

    // Step 8.2. "Otherwise, report the exception given by
    // evaluationStatus.[[Value]] for script." [spec text]
    ScriptModule::ReportException(script_state_.get(), error.V8Value());
  }

  // Step 9. "Clean up after running script with settings." [spec text]
  // Implemented as the ScriptState::Scope destructor.
  return ScriptValue();
}

void ModulatorImplBase::Trace(blink::Visitor* visitor) {
  Modulator::Trace(visitor);
  visitor->Trace(map_);
  visitor->Trace(loader_registry_);
  visitor->Trace(tree_linker_registry_);
  visitor->Trace(script_module_resolver_);
  visitor->Trace(dynamic_module_resolver_);
}

void ModulatorImplBase::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(map_);
  visitor->TraceWrappers(tree_linker_registry_);
  Modulator::TraceWrappers(visitor);
}

}  // namespace blink
