// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worklet_global_scope.h"

#include <memory>
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"
#include "third_party/blink/renderer/core/workers/worklet_module_tree_client.h"
#include "third_party/blink/renderer/core/workers/worklet_pending_tasks.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"

namespace blink {

// Partial implementation of the "set up a worklet environment settings object"
// algorithm:
// https://drafts.css-houdini.org/worklets/#script-settings-for-worklets
WorkletGlobalScope::WorkletGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params,
    v8::Isolate* isolate,
    WorkerReportingProxy& reporting_proxy,
    scoped_refptr<base::SingleThreadTaskRunner> document_loading_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> worklet_loading_task_runner)
    : WorkerOrWorkletGlobalScope(isolate,
                                 creation_params->worker_clients,
                                 reporting_proxy),
      url_(creation_params->script_url),
      user_agent_(creation_params->user_agent),
      document_security_origin_(creation_params->starter_origin),
      document_secure_context_(creation_params->starter_secure_context),
      fetch_coordinator_proxy_(
          WorkerOrWorkletModuleFetchCoordinatorProxy::Create(
              creation_params->module_responses_map,
              std::move(document_loading_task_runner),
              std::move(worklet_loading_task_runner))) {
  // Step 2: "Let inheritedAPIBaseURL be outsideSettings's API base URL."
  // |url_| is the inheritedAPIBaseURL passed from the parent Document.

  // Step 3: "Let origin be a unique opaque origin."
  SetSecurityOrigin(SecurityOrigin::CreateUnique());

  // Step 5: "Let inheritedReferrerPolicy be outsideSettings's referrer policy."
  SetReferrerPolicy(creation_params->referrer_policy);

  // https://drafts.css-houdini.org/worklets/#creating-a-workletglobalscope
  // Step 6: "Invoke the initialize a global object's CSP list algorithm given
  // workletGlobalScope."
  ApplyContentSecurityPolicyFromVector(
      *creation_params->content_security_policy_parsed_headers);

  OriginTrialContext::AddTokens(this,
                                creation_params->origin_trial_tokens.get());
}

WorkletGlobalScope::~WorkletGlobalScope() = default;

ExecutionContext* WorkletGlobalScope::GetExecutionContext() const {
  return const_cast<WorkletGlobalScope*>(this);
}

bool WorkletGlobalScope::IsSecureContext(String& error_message) const {
  // Until there are APIs that are available in worklets and that
  // require a privileged context test that checks ancestors, just do
  // a simple check here.
  if (GetSecurityOrigin()->IsPotentiallyTrustworthy())
    return true;
  error_message = GetSecurityOrigin()->IsPotentiallyTrustworthyErrorMessage();
  return false;
}

// Implementation of the first half of the "fetch and invoke a worklet script"
// algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-and-invoke-a-worklet-script
void WorkletGlobalScope::FetchAndInvokeScript(
    const KURL& module_url_record,
    network::mojom::FetchCredentialsMode credentials_mode,
    scoped_refptr<base::SingleThreadTaskRunner> outside_settings_task_runner,
    WorkletPendingTasks* pending_tasks) {
  DCHECK(IsContextThread());

  // Step 1: "Let insideSettings be the workletGlobalScope's associated
  // environment settings object."
  // Step 2: "Let script by the result of fetch a worklet script given
  // moduleURLRecord, moduleResponsesMap, credentialOptions, outsideSettings,
  // and insideSettings when it asynchronously completes."

  Modulator* modulator = Modulator::From(ScriptController()->GetScriptState());

  // Step 3 to 5 are implemented in
  // WorkletModuleTreeClient::NotifyModuleTreeLoadFinished.
  WorkletModuleTreeClient* client = new WorkletModuleTreeClient(
      modulator, std::move(outside_settings_task_runner), pending_tasks);

  // TODO(nhiroki): Specify an appropriate destination defined in each worklet
  // spec (e.g., "paint worklet", "audio worklet").
  WebURLRequest::RequestContext destination =
      WebURLRequest::kRequestContextScript;
  FetchModuleScript(module_url_record, destination, credentials_mode, client);
}

WorkerOrWorkletModuleFetchCoordinatorProxy*
WorkletGlobalScope::ModuleFetchCoordinatorProxy() const {
  DCHECK(IsContextThread());
  DCHECK(fetch_coordinator_proxy_);
  return fetch_coordinator_proxy_;
}

KURL WorkletGlobalScope::CompleteURL(const String& url) const {
  // Always return a null URL when passed a null string.
  // TODO(ikilpatrick): Should we change the KURL constructor to have this
  // behavior?
  if (url.IsNull())
    return KURL();
  // Always use UTF-8 in Worklets.
  return KURL(BaseURL(), url);
}

void WorkletGlobalScope::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetch_coordinator_proxy_);
  WorkerOrWorkletGlobalScope::Trace(visitor);
}

void WorkletGlobalScope::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  WorkerOrWorkletGlobalScope::TraceWrappers(visitor);
}

}  // namespace blink
