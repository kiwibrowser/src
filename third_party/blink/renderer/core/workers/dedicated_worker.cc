// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/dedicated_worker.h"

#include <memory>
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/public/mojom/interface_provider.mojom-blink.h"
#include "third_party/blink/public/platform/dedicated_worker_factory.mojom-blink.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/core/workers/dedicated_worker_messaging_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_classic_script_loader.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/core/workers/worker_content_settings_client.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"

namespace blink {

namespace {

service_manager::mojom::blink::InterfaceProviderPtrInfo
ConnectToWorkerInterfaceProvider(
    ExecutionContext* execution_context,
    scoped_refptr<const SecurityOrigin> script_origin) {
  mojom::blink::DedicatedWorkerFactoryPtr worker_factory;
  execution_context->GetInterfaceProvider()->GetInterface(&worker_factory);
  service_manager::mojom::blink::InterfaceProviderPtrInfo
      interface_provider_ptr;
  worker_factory->CreateDedicatedWorker(
      script_origin, mojo::MakeRequest(&interface_provider_ptr));
  return interface_provider_ptr;
}

}  // namespace

DedicatedWorker* DedicatedWorker::Create(ExecutionContext* context,
                                         const String& url,
                                         const WorkerOptions& options,
                                         ExceptionState& exception_state) {
  DCHECK(context->IsContextThread());
  UseCounter::Count(context, WebFeature::kWorkerStart);
  if (context->IsContextDestroyed()) {
    exception_state.ThrowDOMException(kInvalidAccessError,
                                      "The context provided is invalid.");
    return nullptr;
  }

  KURL script_url = ResolveURL(context, url, exception_state,
                               WebURLRequest::kRequestContextScript);
  if (!script_url.IsValid()) {
    // Don't throw an exception here because it's already thrown in
    // ResolveURL().
    return nullptr;
  }

  // TODO(nhiroki): Remove this flag check once module loading for
  // DedicatedWorker is enabled by default (https://crbug.com/680046).
  if (options.type() == "module" &&
      !RuntimeEnabledFeatures::ModuleDedicatedWorkerEnabled()) {
    exception_state.ThrowTypeError(
        "Module scripts are not supported on DedicatedWorker yet. You can try "
        "the feature with '--enable-experimental-web-platform-features' flag "
        "(see https://crbug.com/680046)");
    return nullptr;
  }

  if (context->IsWorkerGlobalScope())
    ToWorkerGlobalScope(context)->EnsureFetcher();

  DedicatedWorker* worker = new DedicatedWorker(context, script_url, options);
  worker->Start();
  return worker;
}

DedicatedWorker::DedicatedWorker(ExecutionContext* context,
                                 const KURL& script_url,
                                 const WorkerOptions& options)
    : AbstractWorker(context),
      script_url_(script_url),
      options_(options),
      context_proxy_(new DedicatedWorkerMessagingProxy(context, this)) {
  DCHECK(context->IsContextThread());
  DCHECK(script_url_.IsValid());
  DCHECK(context_proxy_);
}

DedicatedWorker::~DedicatedWorker() {
  DCHECK(!GetExecutionContext() || GetExecutionContext()->IsContextThread());
  context_proxy_->ParentObjectDestroyed();
}

void DedicatedWorker::postMessage(ScriptState* script_state,
                                  scoped_refptr<SerializedScriptValue> message,
                                  const MessagePortArray& ports,
                                  ExceptionState& exception_state) {
  DCHECK(GetExecutionContext()->IsContextThread());
  // Disentangle the port in preparation for sending it to the remote context.
  auto channels = MessagePort::DisentanglePorts(
      ExecutionContext::From(script_state), ports, exception_state);
  if (exception_state.HadException())
    return;
  v8_inspector::V8StackTraceId stack_id =
      ThreadDebugger::From(script_state->GetIsolate())
          ->StoreCurrentStackTrace("Worker.postMessage");
  context_proxy_->PostMessageToWorkerGlobalScope(std::move(message),
                                                 std::move(channels), stack_id);
}

void DedicatedWorker::Start() {
  DCHECK(GetExecutionContext()->IsContextThread());

  v8_inspector::V8StackTraceId stack_id =
      ThreadDebugger::From(ToIsolate(GetExecutionContext()))
          ->StoreCurrentStackTrace("Worker Created");

  if (options_.type() == "classic") {
    network::mojom::FetchRequestMode fetch_request_mode =
        network::mojom::FetchRequestMode::kSameOrigin;
    network::mojom::FetchCredentialsMode fetch_credentials_mode =
        network::mojom::FetchCredentialsMode::kSameOrigin;
    if (script_url_.ProtocolIsData()) {
      fetch_request_mode = network::mojom::FetchRequestMode::kNoCORS;
      fetch_credentials_mode = network::mojom::FetchCredentialsMode::kInclude;
    }
    classic_script_loader_ = WorkerClassicScriptLoader::Create();
    classic_script_loader_->LoadAsynchronously(
        *GetExecutionContext(), script_url_,
        WebURLRequest::kRequestContextWorker, fetch_request_mode,
        fetch_credentials_mode,
        GetExecutionContext()->GetSecurityContext().AddressSpace(),
        WTF::Bind(&DedicatedWorker::OnResponse, WrapPersistent(this)),
        WTF::Bind(&DedicatedWorker::OnFinished, WrapPersistent(this),
                  stack_id));
    return;
  }
  if (options_.type() == "module") {
    // Specify empty source code here because module scripts will be fetched on
    // the worker thread as opposed to classic scripts that are fetched on the
    // main thread.
    context_proxy_->StartWorkerGlobalScope(CreateGlobalScopeCreationParams(),
                                           options_, script_url_, stack_id,
                                           String() /* source_code */);
    return;
  }
  NOTREACHED() << "Invalid type: " << options_.type();
}

void DedicatedWorker::terminate() {
  DCHECK(GetExecutionContext()->IsContextThread());
  context_proxy_->TerminateGlobalScope();
}

BeginFrameProviderParams DedicatedWorker::CreateBeginFrameProviderParams() {
  DCHECK(GetExecutionContext()->IsContextThread());
  // If we don't have a frame or we are not in Document, some of the SinkIds
  // won't be initialized. If that's the case, the Worker will initialize it by
  // itself later.
  BeginFrameProviderParams begin_frame_provider_params;
  if (GetExecutionContext()->IsDocument()) {
    LocalFrame* frame = ToDocument(GetExecutionContext())->GetFrame();
    WebLayerTreeView* layer_tree_view = nullptr;
    if (frame) {
      layer_tree_view =
          frame->GetPage()->GetChromeClient().GetWebLayerTreeView(frame);
      begin_frame_provider_params.parent_frame_sink_id =
          layer_tree_view->GetFrameSinkId();
    }
    begin_frame_provider_params.frame_sink_id =
        Platform::Current()->GenerateFrameSinkId();
  }
  return begin_frame_provider_params;
}

void DedicatedWorker::ContextDestroyed(ExecutionContext*) {
  DCHECK(GetExecutionContext()->IsContextThread());
  if (classic_script_loader_)
    classic_script_loader_->Cancel();
  terminate();
}

bool DedicatedWorker::HasPendingActivity() const {
  DCHECK(!GetExecutionContext() || GetExecutionContext()->IsContextThread());
  // The worker context does not exist while loading, so we must ensure that the
  // worker object is not collected, nor are its event listeners.
  return context_proxy_->HasPendingActivity() || classic_script_loader_;
}

WorkerClients* DedicatedWorker::CreateWorkerClients() {
  WorkerClients* worker_clients = WorkerClients::Create();
  CoreInitializer::GetInstance().ProvideLocalFileSystemToWorker(
      *worker_clients);
  CoreInitializer::GetInstance().ProvideIndexedDBClientToWorker(
      *worker_clients);

  std::unique_ptr<WebContentSettingsClient> client;
  if (GetExecutionContext()->IsDocument()) {
    WebLocalFrameImpl* web_frame = WebLocalFrameImpl::FromFrame(
        ToDocument(GetExecutionContext())->GetFrame());
    client = web_frame->Client()->CreateWorkerContentSettingsClient();
  } else if (GetExecutionContext()->IsWorkerGlobalScope()) {
    WebContentSettingsClient* web_worker_content_settings_client =
        WorkerContentSettingsClient::From(*GetExecutionContext())
            ->GetWebContentSettingsClient();
    if (web_worker_content_settings_client)
      client = web_worker_content_settings_client->Clone();
  }

  ProvideContentSettingsClientToWorker(worker_clients, std::move(client));
  return worker_clients;
}

void DedicatedWorker::OnResponse() {
  DCHECK(GetExecutionContext()->IsContextThread());
  probe::didReceiveScriptResponse(GetExecutionContext(),
                                  classic_script_loader_->Identifier());
}

void DedicatedWorker::OnFinished(const v8_inspector::V8StackTraceId& stack_id) {
  DCHECK(GetExecutionContext()->IsContextThread());
  if (classic_script_loader_->Canceled()) {
    // Do nothing.
  } else if (classic_script_loader_->Failed()) {
    DispatchEvent(Event::CreateCancelable(EventTypeNames::error));
  } else {
    ReferrerPolicy referrer_policy = kReferrerPolicyDefault;
    if (!classic_script_loader_->GetReferrerPolicy().IsNull()) {
      SecurityPolicy::ReferrerPolicyFromHeaderValue(
          classic_script_loader_->GetReferrerPolicy(),
          kDoNotSupportReferrerPolicyLegacyKeywords, &referrer_policy);
    }
    std::unique_ptr<GlobalScopeCreationParams> creation_params =
        CreateGlobalScopeCreationParams();
    creation_params->referrer_policy = referrer_policy;
    context_proxy_->StartWorkerGlobalScope(
        std::move(creation_params), options_, script_url_, stack_id,
        classic_script_loader_->SourceText());
    probe::scriptImported(GetExecutionContext(),
                          classic_script_loader_->Identifier(),
                          classic_script_loader_->SourceText());
  }
  classic_script_loader_ = nullptr;
}

std::unique_ptr<GlobalScopeCreationParams>
DedicatedWorker::CreateGlobalScopeCreationParams() {
  base::UnguessableToken devtools_worker_token;
  std::unique_ptr<WorkerSettings> settings;
  if (GetExecutionContext()->IsDocument()) {
    Document* document = ToDocument(GetExecutionContext());
    devtools_worker_token = document->GetFrame()
                                ? document->GetFrame()->GetDevToolsFrameToken()
                                : base::UnguessableToken::Create();
    settings = std::make_unique<WorkerSettings>(document->GetSettings());
  } else {
    WorkerGlobalScope* worker_global_scope =
        ToWorkerGlobalScope(GetExecutionContext());
    devtools_worker_token = worker_global_scope->GetParentDevToolsToken();
    settings = WorkerSettings::Copy(worker_global_scope->GetWorkerSettings());
  }

  ScriptType script_type = (options_.type() == "classic") ? ScriptType::kClassic
                                                          : ScriptType::kModule;
  return std::make_unique<GlobalScopeCreationParams>(
      script_url_, script_type, GetExecutionContext()->UserAgent(),
      GetExecutionContext()->GetContentSecurityPolicy()->Headers().get(),
      kReferrerPolicyDefault, GetExecutionContext()->GetSecurityOrigin(),
      GetExecutionContext()->IsSecureContext(), CreateWorkerClients(),
      GetExecutionContext()->GetSecurityContext().AddressSpace(),
      OriginTrialContext::GetTokens(GetExecutionContext()).get(),
      devtools_worker_token, std::move(settings), kV8CacheOptionsDefault,
      nullptr /* worklet_module_responses_map */,
      ConnectToWorkerInterfaceProvider(GetExecutionContext(),
                                       SecurityOrigin::Create(script_url_)),
      CreateBeginFrameProviderParams());
}

const AtomicString& DedicatedWorker::InterfaceName() const {
  return EventTargetNames::Worker;
}

void DedicatedWorker::Trace(blink::Visitor* visitor) {
  visitor->Trace(context_proxy_);
  AbstractWorker::Trace(visitor);
}

}  // namespace blink
