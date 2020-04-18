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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_CLIENT_H_

#include <memory>
#include "third_party/blink/public/common/message_port/transferable_message.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_clients_claim_callbacks.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_clients_info.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_skip_waiting_callbacks.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_stream_handle.h"
#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

struct WebPaymentHandlerResponse;
struct WebServiceWorkerClientQueryOptions;
class ExecutionContext;
class WebServiceWorkerContextClient;
class WebServiceWorkerResponse;
class WebURL;
class WorkerClients;

// See WebServiceWorkerContextClient for documentation for the methods in this
// class.
class MODULES_EXPORT ServiceWorkerGlobalScopeClient
    : public GarbageCollected<ServiceWorkerGlobalScopeClient>,
      public Supplement<WorkerClients> {
  USING_GARBAGE_COLLECTED_MIXIN(ServiceWorkerGlobalScopeClient);
  WTF_MAKE_NONCOPYABLE(ServiceWorkerGlobalScopeClient);

 public:
  static const char kSupplementName[];

  explicit ServiceWorkerGlobalScopeClient(WebServiceWorkerContextClient&);

  // Called from ServiceWorkerClients.
  void GetClient(const WebString&,
                 std::unique_ptr<WebServiceWorkerClientCallbacks>);
  void GetClients(const WebServiceWorkerClientQueryOptions&,
                  std::unique_ptr<WebServiceWorkerClientsCallbacks>);
  void OpenWindowForClients(const WebURL&,
                            std::unique_ptr<WebServiceWorkerClientCallbacks>);
  void OpenWindowForPaymentHandler(
      const WebURL&,
      std::unique_ptr<WebServiceWorkerClientCallbacks>);
  void SetCachedMetadata(const WebURL&, const char*, size_t);
  void ClearCachedMetadata(const WebURL&);

  void DidHandleActivateEvent(int event_id,
                              mojom::ServiceWorkerEventStatus,
                              double event_dispatch_time);
  void DidHandleBackgroundFetchAbortEvent(int event_id,
                                          mojom::ServiceWorkerEventStatus,
                                          double event_dispatch_time);
  void DidHandleBackgroundFetchClickEvent(int event_id,
                                          mojom::ServiceWorkerEventStatus,
                                          double event_dispatch_time);
  void DidHandleBackgroundFetchFailEvent(int event_id,
                                         mojom::ServiceWorkerEventStatus,
                                         double event_dispatch_time);
  void DidHandleBackgroundFetchedEvent(int event_id,
                                       mojom::ServiceWorkerEventStatus,
                                       double event_dispatch_time);
  void DidHandleCookieChangeEvent(int event_id,
                                  mojom::ServiceWorkerEventStatus,
                                  double event_dispatch_time);
  void DidHandleExtendableMessageEvent(int event_id,
                                       mojom::ServiceWorkerEventStatus,
                                       double event_dispatch_time);
  void RespondToFetchEventWithNoResponse(int fetch_event_id,
                                         double event_dispatch_time);
  void RespondToFetchEvent(int fetch_event_id,
                           const WebServiceWorkerResponse&,
                           double event_dispatch_time);
  void RespondToFetchEventWithResponseStream(int fetch_event_id,
                                             const WebServiceWorkerResponse&,
                                             WebServiceWorkerStreamHandle*,
                                             double event_dispatch_time);
  void RespondToAbortPaymentEvent(int event_id,
                                  bool abort_payment,
                                  double event_dispatch_time);
  void RespondToCanMakePaymentEvent(int event_id,
                                    bool can_make_payment,
                                    double event_dispatch_time);
  void RespondToPaymentRequestEvent(int event_id,
                                    const WebPaymentHandlerResponse&,
                                    double event_dispatch_time);
  void DidHandleFetchEvent(int fetch_event_id,
                           mojom::ServiceWorkerEventStatus,
                           double event_dispatch_time);
  void DidHandleInstallEvent(int install_event_id,
                             mojom::ServiceWorkerEventStatus,
                             double event_dispatch_time);
  void DidHandleNotificationClickEvent(int event_id,
                                       mojom::ServiceWorkerEventStatus,
                                       double event_dispatch_time);
  void DidHandleNotificationCloseEvent(int event_id,
                                       mojom::ServiceWorkerEventStatus,
                                       double event_dispatch_time);
  void DidHandlePushEvent(int push_event_id,
                          mojom::ServiceWorkerEventStatus,
                          double event_dispatch_time);
  void DidHandleSyncEvent(int sync_event_id,
                          mojom::ServiceWorkerEventStatus,
                          double event_dispatch_time);
  void DidHandleAbortPaymentEvent(int abort_payment_event_id,
                                  mojom::ServiceWorkerEventStatus,
                                  double event_dispatch_time);
  void DidHandleCanMakePaymentEvent(int payment_request_event_id,
                                    mojom::ServiceWorkerEventStatus,
                                    double event_dispatch_time);
  void DidHandlePaymentRequestEvent(int payment_request_event_id,
                                    mojom::ServiceWorkerEventStatus,
                                    double event_dispatch_time);
  void PostMessageToClient(const WebString& client_uuid, TransferableMessage);
  void SkipWaiting(std::unique_ptr<WebServiceWorkerSkipWaitingCallbacks>);
  void Claim(std::unique_ptr<WebServiceWorkerClientsClaimCallbacks>);
  void Focus(const WebString& client_uuid,
             std::unique_ptr<WebServiceWorkerClientCallbacks>);
  void Navigate(const WebString& client_uuid,
                const WebURL&,
                std::unique_ptr<WebServiceWorkerClientCallbacks>);

  static ServiceWorkerGlobalScopeClient* From(ExecutionContext*);

  void Trace(blink::Visitor*) override;

 private:
  WebServiceWorkerContextClient& client_;
};

MODULES_EXPORT void ProvideServiceWorkerGlobalScopeClientToWorker(
    WorkerClients*,
    ServiceWorkerGlobalScopeClient*);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_CLIENT_H_
