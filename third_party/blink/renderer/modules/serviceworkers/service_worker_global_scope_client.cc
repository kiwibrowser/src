/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_client.h"

#include <memory>
#include <utility>
#include "third_party/blink/public/platform/modules/payments/web_payment_handler_response.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/modules/serviceworker/web_service_worker_context_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

namespace blink {

ServiceWorkerGlobalScopeClient::ServiceWorkerGlobalScopeClient(
    WebServiceWorkerContextClient& client)
    : client_(client) {}

void ServiceWorkerGlobalScopeClient::GetClient(
    const WebString& id,
    std::unique_ptr<WebServiceWorkerClientCallbacks> callbacks) {
  client_.GetClient(id, std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::GetClients(
    const WebServiceWorkerClientQueryOptions& options,
    std::unique_ptr<WebServiceWorkerClientsCallbacks> callbacks) {
  client_.GetClients(options, std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::OpenWindowForClients(
    const WebURL& url,
    std::unique_ptr<WebServiceWorkerClientCallbacks> callbacks) {
  client_.OpenNewTab(url, std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::OpenWindowForPaymentHandler(
    const WebURL& url,
    std::unique_ptr<WebServiceWorkerClientCallbacks> callbacks) {
  client_.OpenPaymentHandlerWindow(url, std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::SetCachedMetadata(const WebURL& url,
                                                       const char* data,
                                                       size_t size) {
  client_.SetCachedMetadata(url, data, size);
}

void ServiceWorkerGlobalScopeClient::ClearCachedMetadata(const WebURL& url) {
  client_.ClearCachedMetadata(url);
}

void ServiceWorkerGlobalScopeClient::DidHandleActivateEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleActivateEvent(event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleBackgroundFetchAbortEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleBackgroundFetchAbortEvent(event_id, status,
                                             event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleBackgroundFetchClickEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleBackgroundFetchClickEvent(event_id, status,
                                             event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleBackgroundFetchFailEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleBackgroundFetchFailEvent(event_id, status,
                                            event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleBackgroundFetchedEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleBackgroundFetchedEvent(event_id, status,
                                          event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleCookieChangeEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleCookieChangeEvent(event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleExtendableMessageEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleExtendableMessageEvent(event_id, status,
                                          event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToFetchEventWithNoResponse(
    int fetch_event_id,
    double event_dispatch_time) {
  client_.RespondToFetchEventWithNoResponse(fetch_event_id,
                                            event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToFetchEvent(
    int fetch_event_id,
    const WebServiceWorkerResponse& response,
    double event_dispatch_time) {
  client_.RespondToFetchEvent(fetch_event_id, response, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToFetchEventWithResponseStream(
    int fetch_event_id,
    const WebServiceWorkerResponse& response,
    WebServiceWorkerStreamHandle* stream_handle,
    double event_dispatch_time) {
  client_.RespondToFetchEventWithResponseStream(
      fetch_event_id, response, stream_handle, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToAbortPaymentEvent(
    int event_id,
    bool abort_payment,
    double event_dispatch_time) {
  client_.RespondToAbortPaymentEvent(event_id, abort_payment,
                                     event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToCanMakePaymentEvent(
    int event_id,
    bool response,
    double event_dispatch_time) {
  client_.RespondToCanMakePaymentEvent(event_id, response, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::RespondToPaymentRequestEvent(
    int event_id,
    const WebPaymentHandlerResponse& response,
    double event_dispatch_time) {
  client_.RespondToPaymentRequestEvent(event_id, response, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleFetchEvent(
    int fetch_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleFetchEvent(fetch_event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleInstallEvent(
    int install_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleInstallEvent(install_event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleNotificationClickEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleNotificationClickEvent(event_id, status,
                                          event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleNotificationCloseEvent(
    int event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleNotificationCloseEvent(event_id, status,
                                          event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandlePushEvent(
    int push_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandlePushEvent(push_event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleSyncEvent(
    int sync_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleSyncEvent(sync_event_id, status, event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleAbortPaymentEvent(
    int abort_payment_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleAbortPaymentEvent(abort_payment_event_id, status,
                                     event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandleCanMakePaymentEvent(
    int payment_request_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandleCanMakePaymentEvent(payment_request_event_id, status,
                                       event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::DidHandlePaymentRequestEvent(
    int payment_request_event_id,
    mojom::ServiceWorkerEventStatus status,
    double event_dispatch_time) {
  client_.DidHandlePaymentRequestEvent(payment_request_event_id, status,
                                       event_dispatch_time);
}

void ServiceWorkerGlobalScopeClient::PostMessageToClient(
    const WebString& client_uuid,
    TransferableMessage message) {
  client_.PostMessageToClient(client_uuid, std::move(message));
}

void ServiceWorkerGlobalScopeClient::SkipWaiting(
    std::unique_ptr<WebServiceWorkerSkipWaitingCallbacks> callbacks) {
  client_.SkipWaiting(std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::Claim(
    std::unique_ptr<WebServiceWorkerClientsClaimCallbacks> callbacks) {
  client_.Claim(std::move(callbacks));
}

void ServiceWorkerGlobalScopeClient::Focus(
    const WebString& client_uuid,
    std::unique_ptr<WebServiceWorkerClientCallbacks> callback) {
  client_.Focus(client_uuid, std::move(callback));
}

void ServiceWorkerGlobalScopeClient::Navigate(
    const WebString& client_uuid,
    const WebURL& url,
    std::unique_ptr<WebServiceWorkerClientCallbacks> callback) {
  client_.Navigate(client_uuid, url, std::move(callback));
}

const char ServiceWorkerGlobalScopeClient::kSupplementName[] =
    "ServiceWorkerGlobalScopeClient";

ServiceWorkerGlobalScopeClient* ServiceWorkerGlobalScopeClient::From(
    ExecutionContext* context) {
  // TODO(horo): Replace CHECK() to DCHECK() when crbug.com/749930 is fixed.
  CHECK(context);
  WorkerClients* worker_clients = ToWorkerGlobalScope(context)->Clients();
  CHECK(worker_clients);
  ServiceWorkerGlobalScopeClient* client =
      Supplement<WorkerClients>::From<ServiceWorkerGlobalScopeClient>(
          worker_clients);
  CHECK(client);
  return client;
}

void ServiceWorkerGlobalScopeClient::Trace(blink::Visitor* visitor) {
  Supplement<WorkerClients>::Trace(visitor);
}

void ProvideServiceWorkerGlobalScopeClientToWorker(
    WorkerClients* clients,
    ServiceWorkerGlobalScopeClient* client) {
  clients->ProvideSupplement(client);
}

}  // namespace blink
