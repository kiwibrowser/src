// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_TEST_HELPER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_TEST_HELPER_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_test_sink.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/http/http_response_info.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "url/gurl.h"

class GURL;

namespace content {

struct BackgroundFetchSettledFetch;
class EmbeddedWorkerRegistry;
class EmbeddedWorkerTestHelper;
class MockRenderProcessHost;
class ServiceWorkerContextCore;
class ServiceWorkerContextWrapper;
class ServiceWorkerDispatcherHost;
class TestBrowserContext;
struct PlatformNotificationData;
struct PushEventPayload;

// In-Process EmbeddedWorker test helper.
//
// Usage: create an instance of this class to test browser-side embedded worker
// code without creating a child process.  This class will create a
// ServiceWorkerContextWrapper and ServiceWorkerContextCore for you.
//
// By default this class just notifies back WorkerStarted and WorkerStopped
// for StartWorker and StopWorker requests. The default implementation
// also returns success for event messages (e.g. InstallEvent, FetchEvent).
//
// Alternatively consumers can subclass this helper and override On*()
// methods to add their own logic/verification code.
//
// See embedded_worker_instance_unittest.cc for example usages.
class EmbeddedWorkerTestHelper : public IPC::Sender,
                                 public IPC::Listener {
 public:
  enum class Event { Install, Activate };

  class MockEmbeddedWorkerInstanceClient
      : public mojom::EmbeddedWorkerInstanceClient {
   public:
    explicit MockEmbeddedWorkerInstanceClient(
        base::WeakPtr<EmbeddedWorkerTestHelper> helper);
    ~MockEmbeddedWorkerInstanceClient() override;

    static void Bind(const base::WeakPtr<EmbeddedWorkerTestHelper>& helper,
                     mojom::EmbeddedWorkerInstanceClientRequest request);

   protected:
    // mojom::EmbeddedWorkerInstanceClient implementation.
    void StartWorker(mojom::EmbeddedWorkerStartParamsPtr params) override;
    void StopWorker() override;
    void ResumeAfterDownload() override;
    void AddMessageToConsole(blink::WebConsoleMessage::Level level,
                             const std::string& message) override;
    void BindDevToolsAgent(
        blink::mojom::DevToolsAgentAssociatedRequest request) override {}

    base::WeakPtr<EmbeddedWorkerTestHelper> helper_;
    mojo::Binding<mojom::EmbeddedWorkerInstanceClient> binding_;

    base::Optional<int> embedded_worker_id_;

   private:
    DISALLOW_COPY_AND_ASSIGN(MockEmbeddedWorkerInstanceClient);
  };

  // If |user_data_directory| is empty, the context makes storage stuff in
  // memory.
  explicit EmbeddedWorkerTestHelper(const base::FilePath& user_data_directory);
  // S13nServiceWorker
  EmbeddedWorkerTestHelper(
      const base::FilePath& user_data_directory,
      scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter);
  ~EmbeddedWorkerTestHelper() override;

  // IPC::Sender implementation.
  bool Send(IPC::Message* message) override;

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;

  // Registers a Mojo endpoint object derived from
  // MockEmbeddedWorkerInstanceClient.
  void RegisterMockInstanceClient(
      std::unique_ptr<MockEmbeddedWorkerInstanceClient> client);

  // Registers the dispatcher host for the process to a map managed by this test
  // helper. If there is a existing dispatcher host, it'll replace the existing
  // dispatcher host with the given one. When replacing, this should be called
  // before ServiceWorkerDispatcherHost::Init to allow the old dispatcher host
  // to destruct and remove itself from ServiceWorkerContextCore, since Init
  // adds to context core. If |dispatcher_host| is nullptr, this method just
  // removes the existing dispatcher host from the map.
  void RegisterDispatcherHost(
      int process_id,
      scoped_refptr<ServiceWorkerDispatcherHost> dispatcher_host);

  // Creates and registers a basic dispatcher host for the process if one
  // registered isn't already.
  void EnsureDispatcherHostForProcess(int process_id);

  template <typename MockType, typename... Args>
  MockType* CreateAndRegisterMockInstanceClient(Args&&... args);

  // IPC sink for EmbeddedWorker messages.
  IPC::TestSink* ipc_sink() { return &sink_; }

  std::vector<Event>* dispatched_events() { return &events_; }

  std::vector<std::unique_ptr<MockEmbeddedWorkerInstanceClient>>*
  mock_instance_clients() {
    return &mock_instance_clients_;
  }

  ServiceWorkerContextCore* context();
  ServiceWorkerContextWrapper* context_wrapper() { return wrapper_.get(); }
  void ShutdownContext();

  int GetNextThreadId() { return next_thread_id_++; }

  int mock_render_process_id() const { return mock_render_process_id_; }
  MockRenderProcessHost* mock_render_process_host() {
    return render_process_host_.get();
  }

  // Only used for tests that force creating a new render process.
  int new_render_process_id() const { return new_mock_render_process_id_; }

  TestBrowserContext* browser_context() { return browser_context_.get(); }

  ServiceWorkerDispatcherHost* GetDispatcherHostForProcess(int process_id);

  base::WeakPtr<EmbeddedWorkerTestHelper> AsWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  static net::HttpResponseInfo CreateHttpResponseInfo();

  URLLoaderFactoryGetter* url_loader_factory_getter() {
    return url_loader_factory_getter_.get();
  }

 protected:
  // StartWorker IPC handler routed through MockEmbeddedWorkerInstanceClient.
  // This simulates behaviors in the renderer process. Binds
  // |dispatcher_request| to MockServiceWorkerEventDispatcher by default.
  virtual void OnStartWorker(
      int embedded_worker_id,
      int64_t service_worker_version_id,
      const GURL& scope,
      const GURL& script_url,
      bool pause_after_download,
      mojom::ServiceWorkerEventDispatcherRequest dispatcher_request,
      mojom::ControllerServiceWorkerRequest controller_request,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtrInfo instance_host,
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr provider_info,
      blink::mojom::ServiceWorkerInstalledScriptsInfoPtr
          installed_scripts_info);
  virtual void OnResumeAfterDownload(int embedded_worker_id);
  // StopWorker IPC handler routed through MockEmbeddedWorkerInstanceClient.
  // This calls SimulateWorkerStopped() by default.
  virtual void OnStopWorker(int embedded_worker_id);

  // On*Event handlers. By default they just return success via
  // SimulateSendReplyToBrowser.
  virtual void OnActivateEvent(
      mojom::ServiceWorkerEventDispatcher::DispatchActivateEventCallback
          callback);
  virtual void OnBackgroundFetchAbortEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchAbortEventCallback callback);
  virtual void OnBackgroundFetchClickEvent(
      const std::string& developer_id,
      mojom::BackgroundFetchState state,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchClickEventCallback callback);
  virtual void OnBackgroundFetchFailEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchFailEventCallback callback);
  virtual void OnBackgroundFetchedEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchedEventCallback callback);
  virtual void OnCookieChangeEvent(
      const net::CanonicalCookie& cookie,
      ::network::mojom::CookieChangeCause cause,
      mojom::ServiceWorkerEventDispatcher::DispatchCookieChangeEventCallback
          callback);
  virtual void OnExtendableMessageEvent(
      mojom::ExtendableMessageEventPtr event,
      mojom::ServiceWorkerEventDispatcher::
          DispatchExtendableMessageEventCallback callback);
  virtual void OnInstallEvent(
      mojom::ServiceWorkerEventDispatcher::DispatchInstallEventCallback
          callback);
  virtual void OnFetchEvent(
      int embedded_worker_id,
      const network::ResourceRequest& request,
      mojom::FetchEventPreloadHandlePtr preload_handle,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchFetchEventCallback
          finish_callback);
  virtual void OnNotificationClickEvent(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      int action_index,
      const base::Optional<base::string16>& reply,
      mojom::ServiceWorkerEventDispatcher::
          DispatchNotificationClickEventCallback callback);
  virtual void OnNotificationCloseEvent(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      mojom::ServiceWorkerEventDispatcher::
          DispatchNotificationCloseEventCallback callback);
  virtual void OnPushEvent(
      const PushEventPayload& payload,
      mojom::ServiceWorkerEventDispatcher::DispatchPushEventCallback callback);
  virtual void OnAbortPaymentEvent(
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchAbortPaymentEventCallback
          callback);
  virtual void OnCanMakePaymentEvent(
      payments::mojom::CanMakePaymentEventDataPtr data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchCanMakePaymentEventCallback
          callback);
  virtual void OnPaymentRequestEvent(
      payments::mojom::PaymentRequestEventDataPtr data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchPaymentRequestEventCallback
          callback);

  // These functions simulate making Mojo calls to the browser.
  void SimulateWorkerReadyForInspection(int embedded_worker_id);
  void SimulateWorkerScriptCached(int embedded_worker_id,
                                  base::OnceClosure callback);
  void SimulateWorkerScriptLoaded(int embedded_worker_id);
  void SimulateWorkerThreadStarted(int thread_id, int embedded_worker_id);
  void SimulateWorkerScriptEvaluated(int embedded_worker_id, bool success);
  void SimulateWorkerStarted(int embedded_worker_id);
  void SimulateWorkerStopped(int embedded_worker_id);

  EmbeddedWorkerRegistry* registry();

  blink::mojom::ServiceWorkerHost* GetServiceWorkerHost(
      int embedded_worker_id) {
    return embedded_worker_id_host_map_[embedded_worker_id].get();
  }

 private:
  class MockServiceWorkerEventDispatcher;
  class MockRendererInterface;

  void DidSimulateWorkerScriptCached(int embedded_worker_id,
                                     bool pause_after_download);

  void OnInitializeGlobalScope(
      int embedded_worker_id,
      blink::mojom::ServiceWorkerHostAssociatedPtrInfo service_worker_host,
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration_info);
  void OnStartWorkerStub(mojom::EmbeddedWorkerStartParamsPtr params);
  void OnResumeAfterDownloadStub(int embedded_worker_id);
  void OnStopWorkerStub(int embedded_worker_id);
  void OnActivateEventStub(
      mojom::ServiceWorkerEventDispatcher::DispatchActivateEventCallback
          callback);
  void OnBackgroundFetchAbortEventStub(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchAbortEventCallback callback);
  void OnBackgroundFetchClickEventStub(
      const std::string& developer_id,
      mojom::BackgroundFetchState state,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchClickEventCallback callback);
  void OnBackgroundFetchFailEventStub(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchFailEventCallback callback);
  void OnBackgroundFetchedEventStub(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchedEventCallback callback);
  void OnCookieChangeEventStub(
      const net::CanonicalCookie& cookie,
      ::network::mojom::CookieChangeCause cause,
      mojom::ServiceWorkerEventDispatcher::DispatchCookieChangeEventCallback
          callback);
  void OnExtendableMessageEventStub(
      mojom::ExtendableMessageEventPtr event,
      mojom::ServiceWorkerEventDispatcher::
          DispatchExtendableMessageEventCallback callback);
  void OnInstallEventStub(
      mojom::ServiceWorkerEventDispatcher::DispatchInstallEventCallback
          callback);
  void OnFetchEventStub(
      int embedded_worker_id,
      const network::ResourceRequest& request,
      mojom::FetchEventPreloadHandlePtr preload_handle,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchFetchEventCallback
          finish_callback);
  void OnNotificationClickEventStub(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      int action_index,
      const base::Optional<base::string16>& reply,
      mojom::ServiceWorkerEventDispatcher::
          DispatchNotificationClickEventCallback callback);
  void OnNotificationCloseEventStub(
      const std::string& notification_id,
      const PlatformNotificationData& notification_data,
      mojom::ServiceWorkerEventDispatcher::
          DispatchNotificationCloseEventCallback callback);
  void OnPushEventStub(
      const PushEventPayload& payload,
      mojom::ServiceWorkerEventDispatcher::DispatchPushEventCallback callback);
  void OnAbortPaymentEventStub(
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchAbortPaymentEventCallback
          callback);
  void OnCanMakePaymentEventStub(
      payments::mojom::CanMakePaymentEventDataPtr data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchCanMakePaymentEventCallback
          callback);
  void OnPaymentRequestEventStub(
      payments::mojom::PaymentRequestEventDataPtr data,
      payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
      mojom::ServiceWorkerEventDispatcher::DispatchPaymentRequestEventCallback
          callback);

  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<MockRenderProcessHost> render_process_host_;
  std::unique_ptr<MockRenderProcessHost> new_render_process_host_;

  scoped_refptr<ServiceWorkerContextWrapper> wrapper_;

  IPC::TestSink sink_;

  std::unique_ptr<MockRendererInterface> mock_renderer_interface_;
  std::vector<std::unique_ptr<MockEmbeddedWorkerInstanceClient>>
      mock_instance_clients_;
  size_t mock_instance_clients_next_index_;

  int next_thread_id_;
  int mock_render_process_id_;
  int new_mock_render_process_id_;

  std::map<int /* process_id */, scoped_refptr<ServiceWorkerDispatcherHost>>
      dispatcher_hosts_;

  std::map<int, int64_t> embedded_worker_id_service_worker_version_id_map_;

  std::map<
      int /* embedded_worker_id */,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtr /* instance_host_ptr */>
      embedded_worker_id_instance_host_ptr_map_;
  std::map<int /* embedded_worker_id */, ServiceWorkerRemoteProviderEndpoint>
      embedded_worker_id_remote_provider_map_;
  std::map<int /* embedded_worker_id */,
           blink::mojom::ServiceWorkerInstalledScriptsInfoPtr>
      embedded_worker_id_installed_scripts_info_map_;
  std::map<
      int /* embedded_worker_id */,
      blink::mojom::ServiceWorkerHostAssociatedPtr /* service_worker_host */>
      embedded_worker_id_host_map_;
  std::map<int /* embedded_worker_id */,
           blink::mojom::
               ServiceWorkerRegistrationObjectInfoPtr /* registration_info */>
      embedded_worker_id_registration_info_map_;

  std::vector<Event> events_;
  scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter_;

  base::WeakPtrFactory<EmbeddedWorkerTestHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedWorkerTestHelper);
};

template <typename MockType, typename... Args>
MockType* EmbeddedWorkerTestHelper::CreateAndRegisterMockInstanceClient(
    Args&&... args) {
  std::unique_ptr<MockType> mock =
      std::make_unique<MockType>(std::forward<Args>(args)...);
  MockType* mock_rawptr = mock.get();
  RegisterMockInstanceClient(std::move(mock));
  return mock_rawptr;
}

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_TEST_HELPER_H_
