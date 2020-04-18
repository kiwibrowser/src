// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_CLIENT_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_CLIENT_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "content/common/service_worker/controller_service_worker.mojom.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_provider.mojom.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "ipc/ipc_listener.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_client.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_error.h"
#include "third_party/blink/public/web/modules/serviceworker/web_service_worker_context_client.h"
#include "third_party/blink/public/web/modules/serviceworker/web_service_worker_context_proxy.h"
#include "v8/include/v8.h"

namespace base {
class SingleThreadTaskRunner;
class TaskRunner;
}

namespace blink {
class WebDataConsumerHandle;
struct WebServiceWorkerClientQueryOptions;
class WebServiceWorkerContextProxy;
class WebServiceWorkerProvider;
class WebServiceWorkerResponse;
class WebURLResponse;
}

namespace content {

struct PlatformNotificationData;
struct PushEventPayload;
class EmbeddedWorkerInstanceClientImpl;
class ServiceWorkerNetworkProvider;
class ServiceWorkerProviderContext;
class ServiceWorkerTimeoutTimer;
class WebServiceWorkerImpl;
class WebWorkerFetchContext;

// ServiceWorkerContextClient is a "client" of a service worker execution
// context. It enables communication between the embedder and Blink's
// ServiceWorkerGlobalScope. It is created when the service worker begins
// starting up, and destroyed when the service worker stops. It is owned by
// EmbeddedWorkerInstanceClientImpl's internal WorkerWrapper class.
//
// Unless otherwise noted (here or in base class documentation), all methods are
// called on the worker thread.
class CONTENT_EXPORT ServiceWorkerContextClient
    : public blink::WebServiceWorkerContextClient,
      public mojom::ServiceWorkerEventDispatcher {
 public:
  // Returns a thread-specific client instance.  This does NOT create a
  // new instance.
  static ServiceWorkerContextClient* ThreadSpecificInstance();

  // Called on the main thread.
  // |is_script_streaming| is true if the script is already installed and will
  // be streamed from the browser process.
  ServiceWorkerContextClient(
      int embedded_worker_id,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url,
      bool is_script_streaming,
      mojom::ServiceWorkerEventDispatcherRequest dispatcher_request,
      mojom::ControllerServiceWorkerRequest controller_request,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtrInfo instance_host,
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr provider_info,
      std::unique_ptr<EmbeddedWorkerInstanceClientImpl> embedded_worker_client,
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner);
  ~ServiceWorkerContextClient() override;

  // Called on the main thread.
  void set_blink_initialized_time(base::TimeTicks blink_initialized_time) {
    blink_initialized_time_ = blink_initialized_time;
  }
  void set_start_worker_received_time(
      base::TimeTicks start_worker_received_time) {
    start_worker_received_time_ = start_worker_received_time;
  }

  // Returns the service worker object described by |info|. Creates a new object
  // if needed, or else returns the existing one.
  scoped_refptr<WebServiceWorkerImpl> GetOrCreateServiceWorkerObject(
      blink::mojom::ServiceWorkerObjectInfoPtr info);

  // WebServiceWorkerContextClient overrides.
  void GetClient(
      const blink::WebString& client_id,
      std::unique_ptr<blink::WebServiceWorkerClientCallbacks>) override;
  void GetClients(
      const blink::WebServiceWorkerClientQueryOptions&,
      std::unique_ptr<blink::WebServiceWorkerClientsCallbacks>) override;
  void OpenNewTab(
      const blink::WebURL&,
      std::unique_ptr<blink::WebServiceWorkerClientCallbacks>) override;
  void OpenPaymentHandlerWindow(
      const blink::WebURL&,
      std::unique_ptr<blink::WebServiceWorkerClientCallbacks>) override;
  void SetCachedMetadata(const blink::WebURL&,
                         const char* data,
                         size_t size) override;
  void ClearCachedMetadata(const blink::WebURL&) override;
  void WorkerReadyForInspection() override;
  void WorkerContextFailedToStart() override;
  void WorkerScriptLoaded() override;
  void WorkerContextStarted(
      blink::WebServiceWorkerContextProxy* proxy) override;
  void DidEvaluateClassicScript(bool success) override;
  void DidInitializeWorkerContext(v8::Local<v8::Context> context) override;
  void WillDestroyWorkerContext(v8::Local<v8::Context> context) override;
  void WorkerContextDestroyed() override;
  void CountFeature(blink::mojom::WebFeature feature) override;
  void ReportException(const blink::WebString& error_message,
                       int line_number,
                       int column_number,
                       const blink::WebString& source_url) override;
  void ReportConsoleMessage(int source,
                            int level,
                            const blink::WebString& message,
                            int line_number,
                            const blink::WebString& source_url) override;
  void DidHandleActivateEvent(int request_id,
                              blink::mojom::ServiceWorkerEventStatus status,
                              double dispatch_event_time) override;
  void DidHandleBackgroundFetchAbortEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleBackgroundFetchClickEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleBackgroundFetchFailEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleBackgroundFetchedEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleCookieChangeEvent(int request_id,
                                  blink::mojom::ServiceWorkerEventStatus status,
                                  double dispatch_event_time) override;
  void DidHandleExtendableMessageEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleInstallEvent(int event_id,
                             blink::mojom::ServiceWorkerEventStatus status,
                             double event_dispatch_time) override;
  void RespondToFetchEventWithNoResponse(int fetch_event_id,
                                         double event_dispatch_time) override;
  void RespondToFetchEvent(int fetch_event_id,
                           const blink::WebServiceWorkerResponse& response,
                           double event_dispatch_time) override;
  void RespondToFetchEventWithResponseStream(
      int fetch_event_id,
      const blink::WebServiceWorkerResponse& response,
      blink::WebServiceWorkerStreamHandle* web_body_as_stream,
      double event_dispatch_time) override;
  void DidHandleFetchEvent(int fetch_event_id,
                           blink::mojom::ServiceWorkerEventStatus status,
                           double dispatch_event_time) override;
  void DidHandleNotificationClickEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandleNotificationCloseEvent(
      int request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void DidHandlePushEvent(int request_id,
                          blink::mojom::ServiceWorkerEventStatus status,
                          double dispatch_event_time) override;
  void DidHandleSyncEvent(int request_id,
                          blink::mojom::ServiceWorkerEventStatus status,
                          double dispatch_event_time) override;
  void RespondToAbortPaymentEvent(int event_id,
                                  bool payment_aborted,
                                  double dispatch_event_time) override;
  void DidHandleAbortPaymentEvent(int event_id,
                                  blink::mojom::ServiceWorkerEventStatus status,
                                  double dispatch_event_time) override;
  void RespondToCanMakePaymentEvent(int event_id,
                                    bool can_make_payment,
                                    double dispatch_event_time) override;
  void DidHandleCanMakePaymentEvent(
      int event_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  void RespondToPaymentRequestEvent(
      int payment_request_id,
      const blink::WebPaymentHandlerResponse& response,
      double dispatch_event_time) override;
  void DidHandlePaymentRequestEvent(
      int payment_request_id,
      blink::mojom::ServiceWorkerEventStatus status,
      double dispatch_event_time) override;
  std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
  CreateServiceWorkerNetworkProvider() override;
  std::unique_ptr<blink::WebWorkerFetchContext>
  CreateServiceWorkerFetchContext() override;
  std::unique_ptr<blink::WebServiceWorkerProvider> CreateServiceWorkerProvider()
      override;
  void PostMessageToClient(const blink::WebString& uuid,
                           blink::TransferableMessage message) override;
  void Focus(const blink::WebString& uuid,
             std::unique_ptr<blink::WebServiceWorkerClientCallbacks>) override;
  void Navigate(
      const blink::WebString& uuid,
      const blink::WebURL&,
      std::unique_ptr<blink::WebServiceWorkerClientCallbacks>) override;
  void SkipWaiting(std::unique_ptr<blink::WebServiceWorkerSkipWaitingCallbacks>
                       callbacks) override;
  void Claim(std::unique_ptr<blink::WebServiceWorkerClientsClaimCallbacks>
                 callbacks) override;

  // Dispatches the fetch event if the worker is running normally, and queues it
  // instead if the worker has already requested to be terminated by the
  // browser. If queued, the event will be dispatched once the worker resumes
  // normal operation (if the browser decides not to terminate it, and instead
  // starts another event), or else is dropped if the worker is terminated.
  //
  // This method needs to be used only if the event comes directly from a
  // client, which means it is coming through the ControllerServiceWorkerImpl.
  void DispatchOrQueueFetchEvent(
      mojom::DispatchFetchEventParamsPtr params,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      DispatchFetchEventCallback callback);

 private:
  struct WorkerContextData;
  class NavigationPreloadRequest;
  friend class ControllerServiceWorkerImpl;
  friend class ServiceWorkerContextClientTest;
  friend class WebServiceWorkerImpl;
  FRIEND_TEST_ALL_PREFIXES(
      ServiceWorkerContextClientTest,
      DispatchOrQueueFetchEvent_RequestedTerminationAndDie);
  FRIEND_TEST_ALL_PREFIXES(
      ServiceWorkerContextClientTest,
      DispatchOrQueueFetchEvent_RequestedTerminationAndWakeUp);
  FRIEND_TEST_ALL_PREFIXES(ServiceWorkerContextClientTest,
                           DispatchOrQueueFetchEvent_NotRequestedTermination);

  // Get routing_id for sending message to the ServiceWorkerVersion
  // in the browser process.
  int GetRoutingID() const { return embedded_worker_id_; }

  void SendWorkerStarted();

  // Implements mojom::ServiceWorkerEventDispatcher.
  void InitializeGlobalScope(
      blink::mojom::ServiceWorkerHostAssociatedPtrInfo service_worker_host,
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration_info)
      override;
  void DispatchInstallEvent(
      DispatchInstallEventCallback callback) override;
  void DispatchActivateEvent(DispatchActivateEventCallback callback) override;
  void DispatchBackgroundFetchAbortEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      DispatchBackgroundFetchAbortEventCallback callback) override;
  void DispatchBackgroundFetchClickEvent(
      const std::string& developer_id,
      mojom::BackgroundFetchState state,
      DispatchBackgroundFetchClickEventCallback callback) override;
  void DispatchBackgroundFetchFailEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      DispatchBackgroundFetchFailEventCallback callback) override;
  void DispatchBackgroundFetchedEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      DispatchBackgroundFetchedEventCallback callback) override;
  void DispatchExtendableMessageEvent(
      mojom::ExtendableMessageEventPtr event,
      DispatchExtendableMessageEventCallback callback) override;
  void DispatchFetchEvent(
      mojom::DispatchFetchEventParamsPtr params,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      DispatchFetchEventCallback callback) override;
  void DispatchNotificationClickEvent(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      int action_index,
      const base::Optional<base::string16>& reply,
      DispatchNotificationClickEventCallback callback) override;
  void DispatchNotificationCloseEvent(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      DispatchNotificationCloseEventCallback callback) override;
  void DispatchPushEvent(const PushEventPayload& payload,
                         DispatchPushEventCallback callback) override;
  void DispatchSyncEvent(const std::string& tag,
                         bool last_chance,
                         base::TimeDelta timeout,
                         DispatchSyncEventCallback callback) override;
  void DispatchAbortPaymentEvent(
      int payment_request_id,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      DispatchAbortPaymentEventCallback callback) override;
  void DispatchCanMakePaymentEvent(
      int payment_request_id,
      payments::mojom::CanMakePaymentEventDataPtr event_data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      DispatchCanMakePaymentEventCallback callback) override;
  void DispatchPaymentRequestEvent(
      int payment_request_id,
      payments::mojom::PaymentRequestEventDataPtr event_data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      DispatchPaymentRequestEventCallback callback) override;
  void DispatchCookieChangeEvent(
      const net::CanonicalCookie& cookie,
      ::network::mojom::CookieChangeCause cause,
      DispatchCookieChangeEventCallback callback) override;
  void Ping(PingCallback callback) override;
  void SetIdleTimerDelayToZero() override;

  void OnNotificationClickEvent(
      int request_id,
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      int action_index,
      const base::NullableString16& reply);
  void OnNotificationCloseEvent(
      int request_id,
      const std::string& notification_id,
      const PlatformNotificationData& notification_data);

  void OnFocusClientResponse(
      int request_id,
      const blink::mojom::ServiceWorkerClientInfo& client);
  void OnNavigateClientResponse(
      int request_id,
      const blink::mojom::ServiceWorkerClientInfo& client);
  void OnNavigateClientError(int request_id, const GURL& url);
  // Called to resolve the FetchEvent.preloadResponse promise.
  void OnNavigationPreloadResponse(
      int fetch_event_id,
      std::unique_ptr<blink::WebURLResponse> response,
      std::unique_ptr<blink::WebDataConsumerHandle> data_consumer_handle);
  // Called when the navigation preload request completed. Either
  // OnNavigationPreloadComplete() or OnNavigationPreloadError() must be
  // called to release the preload related resources.
  void OnNavigationPreloadComplete(int fetch_event_id,
                                   base::TimeTicks completion_time,
                                   int64_t encoded_data_length,
                                   int64_t encoded_body_length,
                                   int64_t decoded_body_length);
  // Called when an error occurred while receiving the response of the
  // navigation preload request.
  void OnNavigationPreloadError(
      int fetch_event_id,
      std::unique_ptr<blink::WebServiceWorkerError> error);

  void SetupNavigationPreload(int fetch_event_id,
                              const GURL& url,
                              mojom::FetchEventPreloadHandlePtr preload_handle);

  // Called by ServiceWorkerTimeoutTimer when a certain time has passed since
  // the last task finished.
  void OnIdleTimeout();

  // Returns true if the worker has requested to be terminated by the browser
  // process. It does this due to idle timeout.
  bool RequestedTermination() const;

  // Keeps the mapping from version id to ServiceWorker object.
  void AddServiceWorkerObject(int64_t version_id, WebServiceWorkerImpl* worker);
  void RemoveServiceWorkerObject(int64_t version_id);
  bool ContainsServiceWorkerObjectForTesting(int64_t version_id);

  base::WeakPtr<ServiceWorkerContextClient> GetWeakPtr();

  static void ResetThreadSpecificInstanceForTesting();
  void SetTimeoutTimerForTesting(
      std::unique_ptr<ServiceWorkerTimeoutTimer> timeout_timer);

  const int embedded_worker_id_;
  const int64_t service_worker_version_id_;
  const GURL service_worker_scope_;
  const GURL script_url_;

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::TaskRunner> worker_task_runner_;

  scoped_refptr<ServiceWorkerProviderContext> provider_context_;

  // Not owned; |this| is destroyed when |proxy_| becomes invalid.
  blink::WebServiceWorkerContextProxy* proxy_;

  // These Mojo objects are bound on the worker thread.
  mojom::ServiceWorkerEventDispatcherRequest pending_dispatcher_request_;
  mojom::ControllerServiceWorkerRequest pending_controller_request_;

  // This is bound on the main thread.
  scoped_refptr<mojom::ThreadSafeEmbeddedWorkerInstanceHostAssociatedPtr>
      instance_host_;

  // This is passed to ServiceWorkerNetworkProvider when
  // CreateServiceWorkerNetworkProvider is called.
  std::unique_ptr<ServiceWorkerNetworkProvider> pending_network_provider_;

  // This is valid from the ctor to WorkerContextDestroyed.
  std::unique_ptr<EmbeddedWorkerInstanceClientImpl> embedded_worker_client_;

  blink::mojom::BlobRegistryPtr blob_registry_;

  // Initialized on the worker thread in workerContextStarted and
  // destructed on the worker thread in willDestroyWorkerContext.
  std::unique_ptr<WorkerContextData> context_;

  base::TimeTicks blink_initialized_time_;
  base::TimeTicks start_worker_received_time_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContextClient);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_CLIENT_H_
