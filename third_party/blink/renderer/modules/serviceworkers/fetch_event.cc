// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/fetch_event.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_error.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_for_data_consumer_handle.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/timing/worker_global_scope_performance.h"
#include "third_party/blink/renderer/modules/serviceworkers/fetch_respond_with_observer.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_error.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"

namespace blink {

FetchEvent* FetchEvent::Create(ScriptState* script_state,
                               const AtomicString& type,
                               const FetchEventInit& initializer) {
  return new FetchEvent(script_state, type, initializer, nullptr, nullptr,
                        false);
}

FetchEvent* FetchEvent::Create(ScriptState* script_state,
                               const AtomicString& type,
                               const FetchEventInit& initializer,
                               FetchRespondWithObserver* respond_with_observer,
                               WaitUntilObserver* wait_until_observer,
                               bool navigation_preload_sent) {
  return new FetchEvent(script_state, type, initializer, respond_with_observer,
                        wait_until_observer, navigation_preload_sent);
}

Request* FetchEvent::request() const {
  return request_;
}

String FetchEvent::clientId() const {
  return client_id_;
}

bool FetchEvent::isReload() const {
  UseCounter::Count(GetExecutionContext(), WebFeature::kFetchEventIsReload);
  return is_reload_;
}

void FetchEvent::respondWith(ScriptState* script_state,
                             ScriptPromise script_promise,
                             ExceptionState& exception_state) {
  stopImmediatePropagation();
  if (observer_)
    observer_->RespondWith(script_state, script_promise, exception_state);
}

ScriptPromise FetchEvent::preloadResponse(ScriptState* script_state) {
  return preload_response_property_->Promise(script_state->World());
}

const AtomicString& FetchEvent::InterfaceName() const {
  return EventNames::FetchEvent;
}

bool FetchEvent::HasPendingActivity() const {
  // Prevent V8 from garbage collecting the wrapper object while waiting for the
  // preload response. This is in order to keep the resolver of preloadResponse
  // Promise alive. Note that |preload_response_property_| can be nullptr as
  // GC can run while running the FetchEvent constructor, before the member is
  // set. If it isn't set we treat it as a pending state.
  return !preload_response_property_ ||
         preload_response_property_->GetState() ==
             PreloadResponseProperty::kPending;
}

FetchEvent::FetchEvent(ScriptState* script_state,
                       const AtomicString& type,
                       const FetchEventInit& initializer,
                       FetchRespondWithObserver* respond_with_observer,
                       WaitUntilObserver* wait_until_observer,
                       bool navigation_preload_sent)
    : ExtendableEvent(type, initializer, wait_until_observer),
      ContextClient(ExecutionContext::From(script_state)),
      observer_(respond_with_observer),
      preload_response_property_(new PreloadResponseProperty(
          ExecutionContext::From(script_state),
          this,
          PreloadResponseProperty::kPreloadResponse)) {
  if (!navigation_preload_sent)
    preload_response_property_->ResolveWithUndefined();

  client_id_ = initializer.clientId();
  is_reload_ = initializer.isReload();
  if (initializer.hasRequest()) {
    ScriptState::Scope scope(script_state);
    request_ = initializer.request();
    v8::Local<v8::Value> request = ToV8(request_, script_state);
    v8::Local<v8::Value> event = ToV8(this, script_state);
    if (event.IsEmpty()) {
      // |toV8| can return an empty handle when the worker is terminating.
      // We don't want the renderer to crash in such cases.
      // TODO(yhirano): Replace this branch with an assertion when the
      // graceful shutdown mechanism is introduced.
      return;
    }
    DCHECK(event->IsObject());
    // Sets a hidden value in order to teach V8 the dependency from
    // the event to the request.
    V8PrivateProperty::GetFetchEventRequest(script_state->GetIsolate())
        .Set(event.As<v8::Object>(), request);
    // From the same reason as above, setHiddenValue can return false.
    // TODO(yhirano): Add an assertion that it returns true once the
    // graceful shutdown mechanism is introduced.
  }
}

FetchEvent::~FetchEvent() = default;

void FetchEvent::OnNavigationPreloadResponse(
    ScriptState* script_state,
    std::unique_ptr<WebURLResponse> response,
    std::unique_ptr<WebDataConsumerHandle> data_consume_handle) {
  if (!script_state->ContextIsValid())
    return;
  DCHECK(preload_response_property_);
  DCHECK(!preload_response_);
  ScriptState::Scope scope(script_state);
  preload_response_ = std::move(response);
  // TODO(ricea): Verify that this response can't be aborted from JS.
  FetchResponseData* response_data =
      data_consume_handle
          ? FetchResponseData::CreateWithBuffer(new BodyStreamBuffer(
                script_state,
                new BytesConsumerForDataConsumerHandle(
                    ExecutionContext::From(script_state),
                    std::move(data_consume_handle)),
                new AbortSignal(ExecutionContext::From(script_state))))
          : FetchResponseData::Create();
  Vector<KURL> url_list(1);
  url_list[0] = preload_response_->Url();
  response_data->SetURLList(url_list);
  response_data->SetStatus(preload_response_->HttpStatusCode());
  response_data->SetStatusMessage(preload_response_->HttpStatusText());
  response_data->SetResponseTime(
      preload_response_->ToResourceResponse().ResponseTime());
  const HTTPHeaderMap& headers(
      preload_response_->ToResourceResponse().HttpHeaderFields());
  for (const auto& header : headers) {
    response_data->HeaderList()->Append(header.key, header.value);
  }
  FetchResponseData* tainted_response =
      NetworkUtils::IsRedirectResponseCode(preload_response_->HttpStatusCode())
          ? response_data->CreateOpaqueRedirectFilteredResponse()
          : response_data->CreateBasicFilteredResponse();
  preload_response_property_->Resolve(
      Response::Create(ExecutionContext::From(script_state), tainted_response));
}

void FetchEvent::OnNavigationPreloadError(
    ScriptState* script_state,
    std::unique_ptr<WebServiceWorkerError> error) {
  if (!script_state->ContextIsValid())
    return;
  DCHECK(preload_response_property_);
  if (preload_response_property_->GetState() !=
      PreloadResponseProperty::kPending) {
    return;
  }
  preload_response_property_->Reject(
      ServiceWorkerError::Take(nullptr, *error.get()));
}

void FetchEvent::OnNavigationPreloadComplete(
    WorkerGlobalScope* worker_global_scope,
    TimeTicks completion_time,
    int64_t encoded_data_length,
    int64_t encoded_body_length,
    int64_t decoded_body_length) {
  DCHECK(preload_response_);
  std::unique_ptr<WebURLResponse> response = std::move(preload_response_);
  ResourceResponse resource_response = response->ToResourceResponse();
  resource_response.SetEncodedDataLength(encoded_data_length);
  resource_response.SetEncodedBodyLength(encoded_body_length);
  resource_response.SetDecodedBodyLength(decoded_body_length);
  // According to the Resource Timing spec, the initiator type of
  // navigation preload request is "navigation".
  scoped_refptr<ResourceTimingInfo> info = ResourceTimingInfo::Create(
      "navigation", resource_response.GetResourceLoadTiming()->RequestTime(),
      false /* is_main_resource */);
  info->SetNegativeAllowed(true);
  info->SetLoadFinishTime(completion_time);
  info->SetInitialURL(request_->url());
  info->SetFinalResponse(resource_response);
  info->AddFinalTransferSize(encoded_data_length);
  WorkerGlobalScopePerformance::performance(*worker_global_scope)
      ->GenerateAndAddResourceTiming(*info);
}

void FetchEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(observer_);
  visitor->Trace(request_);
  visitor->Trace(preload_response_property_);
  ExtendableEvent::Trace(visitor);
  ContextClient::Trace(visitor);
}

}  // namespace blink
