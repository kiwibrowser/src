// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/shared_worker/embedded_shared_worker_stub.h"

#include <stdint.h>
#include <utility>

#include "base/feature_list.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/scoped_child_process_reference.h"
#include "content/common/possibly_associated_wrapper_shared_url_loader_factory.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/common/appcache_info.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/origin_util.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/appcache/appcache_dispatcher.h"
#include "content/renderer/appcache/web_application_cache_host_impl.h"
#include "content/renderer/loader/child_url_loader_factory_bundle.h"
#include "content/renderer/loader/request_extra_data.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/renderer_blink_platform_impl.h"
#include "content/renderer/service_worker/service_worker_network_provider.h"
#include "content/renderer/service_worker/service_worker_provider_context.h"
#include "content/renderer/service_worker/worker_fetch_context_impl.h"
#include "ipc/ipc_message_macros.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_shared_worker.h"
#include "third_party/blink/public/web/web_shared_worker_client.h"
#include "url/origin.h"

namespace content {

namespace {

class SharedWorkerWebApplicationCacheHostImpl
    : public WebApplicationCacheHostImpl {
 public:
  SharedWorkerWebApplicationCacheHostImpl(
      blink::WebApplicationCacheHostClient* client)
      : WebApplicationCacheHostImpl(
            client,
            RenderThreadImpl::current()->appcache_dispatcher()->backend_proxy(),
            kAppCacheNoHostId) {}

  // Main resource loading is different for workers. The main resource is
  // loaded by the worker using WorkerClassicScriptLoader.
  // These overrides are stubbed out.
  void WillStartMainResourceRequest(
      const blink::WebURL& url,
      const blink::WebString& method,
      const WebApplicationCacheHost* spawning_host) override {}
  void DidReceiveResponseForMainResource(
      const blink::WebURLResponse&) override {}
  void DidReceiveDataForMainResource(const char* data, unsigned len) override {}
  void DidFinishLoadingMainResource(bool success) override {}

  // Cache selection is also different for workers. We know at construction
  // time what cache to select and do so then.
  // These overrides are stubbed out.
  void SelectCacheWithoutManifest() override {}
  bool SelectCacheWithManifest(const blink::WebURL& manifestURL) override {
    return true;
  }
};

// Called on the main thread only and blink owns it.
class WebServiceWorkerNetworkProviderForSharedWorker
    : public blink::WebServiceWorkerNetworkProvider {
 public:
  WebServiceWorkerNetworkProviderForSharedWorker(
      std::unique_ptr<ServiceWorkerNetworkProvider> provider,
      bool is_secure_context)
      : provider_(std::move(provider)), is_secure_context_(is_secure_context) {}

  // Blink calls this method for each request starting with the main script,
  // we tag them with the provider id.
  void WillSendRequest(blink::WebURLRequest& request) override {
    auto extra_data = std::make_unique<RequestExtraData>();
    extra_data->set_service_worker_provider_id(provider_->provider_id());
    extra_data->set_initiated_in_secure_context(is_secure_context_);
    request.SetExtraData(std::move(extra_data));
    // If the provider does not have a controller at this point, the renderer
    // expects subresource requests to never be handled by a controlling service
    // worker, so set |skip_service_worker| to skip service workers here.
    // Otherwise, a service worker that is in the process of becoming the
    // controller (i.e., via claim()) on the browser-side could handle the
    // request and break the assumptions of the renderer.
    if (request.GetRequestContext() !=
            blink::WebURLRequest::kRequestContextSharedWorker &&
        !provider_->IsControlledByServiceWorker()) {
      request.SetSkipServiceWorker(true);
    }
  }

  int ProviderID() const override { return provider_->provider_id(); }

  bool HasControllerServiceWorker() override {
    return provider_->IsControlledByServiceWorker();
  }

  int64_t ControllerServiceWorkerID() override {
    if (provider_->context())
      return provider_->context()->GetControllerVersionId();
    return blink::mojom::kInvalidServiceWorkerVersionId;
  }

  ServiceWorkerNetworkProvider* provider() { return provider_.get(); }

  std::unique_ptr<blink::WebURLLoader> CreateURLLoader(
      const blink::WebURLRequest& request,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override {
    // S13nServiceWorker:
    // We only install our own URLLoader if Servicification is enabled.
    if (!ServiceWorkerUtils::IsServicificationEnabled())
      return nullptr;

    RenderThreadImpl* render_thread = RenderThreadImpl::current();
    // RenderThreadImpl is nullptr in some tests.
    if (!render_thread) {
      return nullptr;
    }
    // If the request is for the main script, use the script_loader_factory.
    if (provider_->script_loader_factory() &&
        request.GetRequestContext() ==
            blink::WebURLRequest::kRequestContextSharedWorker) {
      // TODO(crbug.com/796425): Temporarily wrap the raw
      // mojom::URLLoaderFactory pointer into SharedURLLoaderFactory.
      return std::make_unique<WebURLLoaderImpl>(
          render_thread->resource_dispatcher(), std::move(task_runner),
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              provider_->script_loader_factory()));
    }

    // Otherwise, it's an importScript. Use the subresource loader factory.
    if (!provider_->context() ||
        !provider_->context()->GetSubresourceLoaderFactory()) {
      return nullptr;
    }

    // If it's not for HTTP or HTTPS, no need to intercept the request.
    if (!GURL(request.Url()).SchemeIsHTTPOrHTTPS())
      return nullptr;

    // If GetSkipServiceWorker() returns true, do not intercept the request.
    if (request.GetSkipServiceWorker())
      return nullptr;

    // Create our own SubresourceLoader to route the request
    // to the controller ServiceWorker.
    // TODO(crbug.com/796425): Temporarily wrap the raw mojom::URLLoaderFactory
    // pointer into SharedURLLoaderFactory.
    return std::make_unique<WebURLLoaderImpl>(
        RenderThreadImpl::current()->resource_dispatcher(),
        std::move(task_runner),
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            provider_->context()->GetSubresourceLoaderFactory()));
  }

 private:
  std::unique_ptr<ServiceWorkerNetworkProvider> provider_;
  const bool is_secure_context_;
};

}  // namespace

EmbeddedSharedWorkerStub::EmbeddedSharedWorkerStub(
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
    service_manager::mojom::InterfaceProviderPtr interface_provider)
    : binding_(this, std::move(request)),
      host_(std::move(host)),
      name_(info->name),
      url_(info->url) {
  impl_ = blink::WebSharedWorker::Create(this);
  if (pause_on_start) {
    // Pause worker context when it starts and wait until either DevTools client
    // is attached or explicit resume notification is received.
    impl_->PauseWorkerContextOnStart();
  }

  service_worker_provider_info_ = std::move(service_worker_provider_info);
  script_loader_factory_info_ = std::move(script_loader_factory_info);

  impl_->StartWorkerContext(
      url_, blink::WebString::FromUTF8(name_),
      blink::WebString::FromUTF8(info->content_security_policy),
      info->content_security_policy_type, info->creation_address_space,
      devtools_worker_token, content_settings.PassInterface().PassHandle(),
      interface_provider.PassInterface().PassHandle());

  // If the host drops its connection, then self-destruct.
  binding_.set_connection_error_handler(base::BindOnce(
      &EmbeddedSharedWorkerStub::Terminate, base::Unretained(this)));
}

EmbeddedSharedWorkerStub::~EmbeddedSharedWorkerStub() {
  // Destruction closes our connection to the host, triggering the host to
  // cleanup and notify clients of this worker going away.
}

void EmbeddedSharedWorkerStub::WorkerReadyForInspection() {
  host_->OnReadyForInspection();
}

void EmbeddedSharedWorkerStub::WorkerScriptLoaded() {
  host_->OnScriptLoaded();
  running_ = true;
  // Process any pending connections.
  for (auto& item : pending_channels_)
    ConnectToChannel(item.first, std::move(item.second));
  pending_channels_.clear();
}

void EmbeddedSharedWorkerStub::WorkerScriptLoadFailed() {
  host_->OnScriptLoadFailed();
  pending_channels_.clear();
  delete this;
}

void EmbeddedSharedWorkerStub::CountFeature(blink::mojom::WebFeature feature) {
  host_->OnFeatureUsed(feature);
}

void EmbeddedSharedWorkerStub::WorkerContextClosed() {
  host_->OnContextClosed();
}

void EmbeddedSharedWorkerStub::WorkerContextDestroyed() {
  delete this;
}

void EmbeddedSharedWorkerStub::SelectAppCacheID(long long app_cache_id) {
  if (app_cache_host_) {
    // app_cache_host_ could become stale as it's owned by blink's
    // DocumentLoader. This method is assumed to be called while it's valid.
    app_cache_host_->backend()->SelectCacheForSharedWorker(
        app_cache_host_->host_id(), app_cache_id);
  }
}

blink::WebNotificationPresenter*
EmbeddedSharedWorkerStub::NotificationPresenter() {
  // TODO(horo): delete this method if we have no plan to implement this.
  NOTREACHED();
  return nullptr;
}

std::unique_ptr<blink::WebApplicationCacheHost>
EmbeddedSharedWorkerStub::CreateApplicationCacheHost(
    blink::WebApplicationCacheHostClient* client) {
  std::unique_ptr<WebApplicationCacheHostImpl> host =
      std::make_unique<SharedWorkerWebApplicationCacheHostImpl>(client);
  app_cache_host_ = host.get();
  return std::move(host);
}

std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
EmbeddedSharedWorkerStub::CreateServiceWorkerNetworkProvider() {
  scoped_refptr<network::SharedURLLoaderFactory> fallback_factory;
  // current() may be null in tests.
  if (RenderThreadImpl* render_thread = RenderThreadImpl::current()) {
    scoped_refptr<ChildURLLoaderFactoryBundle> bundle =
        render_thread->blink_platform_impl()
            ->CreateDefaultURLLoaderFactoryBundle();
    fallback_factory = network::SharedURLLoaderFactory::Create(
        bundle->CloneWithoutDefaultFactory());
  }

  std::unique_ptr<ServiceWorkerNetworkProvider> provider =
      ServiceWorkerNetworkProvider::CreateForSharedWorker(
          std::move(service_worker_provider_info_),
          std::move(script_loader_factory_info_), std::move(fallback_factory));
  return std::make_unique<WebServiceWorkerNetworkProviderForSharedWorker>(
      std::move(provider), IsOriginSecure(url_));
}

void EmbeddedSharedWorkerStub::WaitForServiceWorkerControllerInfo(
    blink::WebServiceWorkerNetworkProvider* web_network_provider,
    base::OnceClosure callback) {
  ServiceWorkerProviderContext* context =
      static_cast<WebServiceWorkerNetworkProviderForSharedWorker*>(
          web_network_provider)
          ->provider()
          ->context();
  context->PingContainerHost(std::move(callback));
}

std::unique_ptr<blink::WebWorkerFetchContext>
EmbeddedSharedWorkerStub::CreateWorkerFetchContext(
    blink::WebServiceWorkerNetworkProvider* web_network_provider) {
  DCHECK(web_network_provider);
  ServiceWorkerProviderContext* context =
      static_cast<WebServiceWorkerNetworkProviderForSharedWorker*>(
          web_network_provider)
          ->provider()
          ->context();
  mojom::ServiceWorkerWorkerClientRequest request =
      context->CreateWorkerClientRequest();

  mojom::ServiceWorkerContainerHostPtrInfo container_host_ptr_info;
  // TODO(horo): Use this host pointer also when S13nServiceWorker is not
  // enabled once we support navigator.serviceWorker on shared workers:
  // crbug.com/371690. Currently we use this only to call
  // GetControllerServiceWorker() from the worker thread if S13nServiceWorker
  // is enabled.
  if (ServiceWorkerUtils::IsServicificationEnabled())
    container_host_ptr_info = context->CloneContainerHostPtrInfo();

  scoped_refptr<ChildURLLoaderFactoryBundle> url_loader_factory_bundle =
      RenderThreadImpl::current()
          ->blink_platform_impl()
          ->CreateDefaultURLLoaderFactoryBundle();
  auto worker_fetch_context = std::make_unique<WorkerFetchContextImpl>(
      std::move(request), std::move(container_host_ptr_info),
      url_loader_factory_bundle->Clone(),
      url_loader_factory_bundle->CloneWithoutDefaultFactory(),
      GetContentClient()->renderer()->CreateURLLoaderThrottleProvider(
          URLLoaderThrottleProviderType::kWorker),
      GetContentClient()
          ->renderer()
          ->CreateWebSocketHandshakeThrottleProvider(),
      ChildThreadImpl::current()->thread_safe_sender(),
      RenderThreadImpl::current()->GetIOTaskRunner());

  // TODO(horo): To get the correct first_party_to_cookies for the shared
  // worker, we need to check the all documents bounded by the shared worker.
  // (crbug.com/723553)
  // https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-07#section-2.1.2
  worker_fetch_context->set_site_for_cookies(url_);
  // TODO(horo): Currently we treat the worker context as secure if the origin
  // of the shared worker script url is secure. But according to the spec, if
  // the creation context is not secure, we should treat the worker as
  // non-secure. crbug.com/723575
  // https://w3c.github.io/webappsec-secure-contexts/#examples-shared-workers
  worker_fetch_context->set_is_secure_context(IsOriginSecure(url_));
  worker_fetch_context->set_origin_url(url_.GetOrigin());
  if (web_network_provider) {
    worker_fetch_context->set_service_worker_provider_id(
        web_network_provider->ProviderID());
    worker_fetch_context->set_is_controlled_by_service_worker(
        web_network_provider->HasControllerServiceWorker());
  }
  worker_fetch_context->set_client_id(context->client_id());
  return std::move(worker_fetch_context);
}

void EmbeddedSharedWorkerStub::ConnectToChannel(
    int connection_request_id,
    blink::MessagePortChannel channel) {
  impl_->Connect(std::move(channel));
  host_->OnConnected(connection_request_id);
}

void EmbeddedSharedWorkerStub::Connect(int connection_request_id,
                                       mojo::ScopedMessagePipeHandle port) {
  blink::MessagePortChannel channel(std::move(port));
  if (running_) {
    ConnectToChannel(connection_request_id, std::move(channel));
  } else {
    // If two documents try to load a SharedWorker at the same time, the
    // mojom::SharedWorker::Connect() for one of the documents can come in
    // before the worker is started. Just queue up the connect and deliver it
    // once the worker starts.
    pending_channels_.emplace_back(connection_request_id, std::move(channel));
  }
}

void EmbeddedSharedWorkerStub::Terminate() {
  // After this we wouldn't get any IPC for this stub.
  running_ = false;
  impl_->TerminateWorkerContext();
}

void EmbeddedSharedWorkerStub::BindDevToolsAgent(
    blink::mojom::DevToolsAgentAssociatedRequest request) {
  impl_->BindDevToolsAgent(request.PassHandle());
}

}  // namespace content
