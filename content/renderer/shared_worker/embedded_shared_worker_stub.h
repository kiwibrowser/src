// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SHARED_WORKER_EMBEDDED_SHARED_WORKER_STUB_H_
#define CONTENT_RENDERER_SHARED_WORKER_EMBEDDED_SHARED_WORKER_STUB_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "content/child/scoped_child_process_reference.h"
#include "content/common/service_worker/service_worker_provider.mojom.h"
#include "content/common/shared_worker/shared_worker.mojom.h"
#include "content/common/shared_worker/shared_worker_host.mojom.h"
#include "content/common/shared_worker/shared_worker_info.mojom.h"
#include "content/renderer/child_message_filter.h"
#include "ipc/ipc_listener.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/service_manager/public/mojom/interface_provider.mojom.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/devtools_agent.mojom.h"
#include "third_party/blink/public/web/web_shared_worker_client.h"
#include "third_party/blink/public/web/worker_content_settings_proxy.mojom.h"
#include "url/gurl.h"

namespace blink {
class WebApplicationCacheHost;
class WebApplicationCacheHostClient;
class WebNotificationPresenter;
class WebSharedWorker;
}

namespace blink {
class MessagePortChannel;
}

namespace content {
class WebApplicationCacheHostImpl;

// A stub class to receive IPC from browser process and talk to
// blink::WebSharedWorker. Implements blink::WebSharedWorkerClient.
// This class is self-destructed (no one explicitly owns this). It deletes
// itself when either one of following methods is called by
// blink::WebSharedWorker:
// - WorkerScriptLoadFailed() or
// - WorkerContextDestroyed()
//
// This class owns blink::WebSharedWorker.
class EmbeddedSharedWorkerStub : public blink::WebSharedWorkerClient,
                                 public mojom::SharedWorker {
 public:
  EmbeddedSharedWorkerStub(
      mojom::SharedWorkerInfoPtr info,
      bool pause_on_start,
      const base::UnguessableToken& devtools_worker_token,
      blink::mojom::WorkerContentSettingsProxyPtr content_settings,
      mojom::ServiceWorkerProviderInfoForSharedWorkerPtr
          service_worker_provider_info,
      network::mojom::URLLoaderFactoryAssociatedPtrInfo
          script_loader_factory_info,
      mojom::SharedWorkerHostPtr host,
      mojom::SharedWorkerRequest request,
      service_manager::mojom::InterfaceProviderPtr interface_provider);
  ~EmbeddedSharedWorkerStub() override;

  // blink::WebSharedWorkerClient implementation.
  void CountFeature(blink::mojom::WebFeature feature) override;
  void WorkerContextClosed() override;
  void WorkerContextDestroyed() override;
  void WorkerReadyForInspection() override;
  void WorkerScriptLoaded() override;
  void WorkerScriptLoadFailed() override;
  void SelectAppCacheID(long long) override;
  blink::WebNotificationPresenter* NotificationPresenter() override;
  std::unique_ptr<blink::WebApplicationCacheHost> CreateApplicationCacheHost(
      blink::WebApplicationCacheHostClient*) override;
  std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
  CreateServiceWorkerNetworkProvider() override;
  std::unique_ptr<blink::WebWorkerFetchContext> CreateWorkerFetchContext(
      blink::WebServiceWorkerNetworkProvider*) override;
  void WaitForServiceWorkerControllerInfo(
      blink::WebServiceWorkerNetworkProvider* web_network_provider,
      base::OnceClosure callback) override;

 private:
  // WebSharedWorker will own |channel|.
  void ConnectToChannel(int connection_request_id,
                        blink::MessagePortChannel channel);

  // mojom::SharedWorker methods:
  void Connect(int connection_request_id,
               mojo::ScopedMessagePipeHandle port) override;
  void Terminate() override;
  void BindDevToolsAgent(
      blink::mojom::DevToolsAgentAssociatedRequest request) override;

  mojo::Binding<mojom::SharedWorker> binding_;
  mojom::SharedWorkerHostPtr host_;
  const std::string name_;
  bool running_ = false;
  GURL url_;
  std::unique_ptr<blink::WebSharedWorker> impl_;

  using PendingChannel =
      std::pair<int /* connection_request_id */, blink::MessagePortChannel>;
  std::vector<PendingChannel> pending_channels_;

  ScopedChildProcessReference process_ref_;
  WebApplicationCacheHostImpl* app_cache_host_ = nullptr;  // Not owned.

  // S13nServiceWorker: The info needed to connect to the
  // ServiceWorkerProviderHost on the browser.
  mojom::ServiceWorkerProviderInfoForSharedWorkerPtr
      service_worker_provider_info_;
  // NetworkService: The URLLoaderFactory used for loading the shared worker
  // script.
  network::mojom::URLLoaderFactoryAssociatedPtrInfo script_loader_factory_info_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedSharedWorkerStub);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SHARED_WORKER_EMBEDDED_SHARED_WORKER_STUB_H_
