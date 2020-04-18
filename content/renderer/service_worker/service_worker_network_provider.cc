// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_network_provider.h"

#include "base/atomic_sequence_num.h"
#include "base/single_thread_task_runner.h"
#include "content/common/navigation_params.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_provider_host_info.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/origin_util.h"
#include "content/renderer/loader/request_extra_data.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/renderer_blink_platform_impl.h"
#include "ipc/ipc_sync_channel.h"
#include "mojo/public/cpp/bindings/associated_group.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"
#include "third_party/blink/public/common/frame/sandbox_flags.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {

namespace {

// Must be unique in the child process.
int GetNextProviderId() {
  static base::AtomicSequenceNumber sequence;
  return sequence.GetNext();  // We start at zero.
}

// Returns whether it's possible for a document whose frame is a descendant of
// |frame| to be a secure context, not considering scheme exceptions (since any
// document can be a secure context if it has a scheme exception). See
// Document::isSecureContextImpl for more details.
bool IsFrameSecure(blink::WebFrame* frame) {
  while (frame) {
    if (!frame->GetSecurityOrigin().IsPotentiallyTrustworthy())
      return false;
    frame = frame->Parent();
  }
  return true;
}

// An WebServiceWorkerNetworkProvider for frame. This wraps
// ServiceWorkerNetworkProvider implementation and is owned by blink.
class WebServiceWorkerNetworkProviderForFrame
    : public blink::WebServiceWorkerNetworkProvider {
 public:
  WebServiceWorkerNetworkProviderForFrame(
      std::unique_ptr<ServiceWorkerNetworkProvider> provider)
      : provider_(std::move(provider)) {}

  void WillSendRequest(blink::WebURLRequest& request) override {
    if (!request.GetExtraData())
      request.SetExtraData(std::make_unique<RequestExtraData>());
    auto* extra_data = static_cast<RequestExtraData*>(request.GetExtraData());
    extra_data->set_service_worker_provider_id(provider_->provider_id());

    // If the provider does not have a controller at this point, the renderer
    // expects the request to never be handled by a service worker, so call
    // SetSkipServiceWorker() with true to skip service workers here. Otherwise,
    // a service worker that is in the process of becoming the controller (i.e.,
    // via claim()) on the browser-side could handle the request and break the
    // assumptions of the renderer.
    if (request.GetFrameType() !=
            network::mojom::RequestContextFrameType::kTopLevel &&
        request.GetFrameType() !=
            network::mojom::RequestContextFrameType::kNested &&
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
    // RenderThreadImpl is nullptr in some tests.
    if (!RenderThreadImpl::current())
      return nullptr;

    // S13nServiceWorker:
    // We only install our own URLLoader if Servicification is enabled.
    if (!ServiceWorkerUtils::IsServicificationEnabled())
      return nullptr;

    // We need SubresourceLoaderFactory populated in order to create our own
    // URLLoader for subresource loading.
    if (!provider_->context() ||
        !provider_->context()->GetSubresourceLoaderFactory())
      return nullptr;

    // If the URL is not http(s) or otherwise whitelisted, do not intercept the
    // request. Schemes like 'blob' and 'file' are not eligible to be
    // intercepted by service workers.
    // TODO(falken): Let ServiceWorkerSubresourceLoaderFactory handle the
    // request and move this check there (i.e., for such URLs, it should use
    // its fallback factory).
    const GURL gurl(request.Url());
    if (!gurl.SchemeIsHTTPOrHTTPS() && !OriginCanAccessServiceWorkers(gurl))
      return nullptr;

    // If GetSkipServiceWorker() returns true, do not intercept the request.
    if (request.GetSkipServiceWorker())
      return nullptr;

    // Create our own SubresourceLoader to route the request to the controller
    // ServiceWorker.
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
};

}  // namespace

// static
std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
ServiceWorkerNetworkProvider::CreateForNavigation(
    int route_id,
    const RequestNavigationParams& request_params,
    blink::WebLocalFrame* frame,
    bool content_initiated,
    mojom::ControllerServiceWorkerInfoPtr controller_info,
    scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory) {
  // Determine if a ServiceWorkerNetworkProvider should be created and properly
  // initialized for the navigation. A default ServiceWorkerNetworkProvider
  // will always be created since it is expected in a certain number of places,
  // however it will have an invalid id.
  bool should_create_provider = false;
  int provider_id = kInvalidServiceWorkerProviderId;
  if (content_initiated) {
    should_create_provider =
        ((frame->EffectiveSandboxFlags() & blink::WebSandboxFlags::kOrigin) !=
         blink::WebSandboxFlags::kOrigin);
  } else {
    should_create_provider = request_params.should_create_service_worker;
    provider_id = request_params.service_worker_provider_id;
  }

  // If we shouldn't create a real ServiceWorkerNetworkProvider, return one with
  // an invalid id.
  if (!should_create_provider) {
    return std::make_unique<WebServiceWorkerNetworkProviderForFrame>(
        base::WrapUnique(new ServiceWorkerNetworkProvider()));
  }

  // Otherwise, create the ServiceWorkerNetworkProvider.

  // Ideally Document::IsSecureContext would be called here, but the document is
  // not created yet, and due to redirects the URL may change. So pass
  // is_parent_frame_secure to the browser process, so it can determine the
  // context security when deciding whether to allow a service worker to control
  // the document.
  const bool is_parent_frame_secure = IsFrameSecure(frame->Parent());

  // If the browser process did not assign a provider id already, assign one
  // now (see class comments for content::ServiceWorkerProviderHost).
  DCHECK(ServiceWorkerUtils::IsBrowserAssignedProviderId(provider_id) ||
         provider_id == kInvalidServiceWorkerProviderId);
  if (provider_id == kInvalidServiceWorkerProviderId)
    provider_id = GetNextProviderId();

  auto provider = base::WrapUnique(new ServiceWorkerNetworkProvider(
      route_id, blink::mojom::ServiceWorkerProviderType::kForWindow,
      provider_id, is_parent_frame_secure, std::move(controller_info),
      std::move(fallback_loader_factory)));
  return std::make_unique<WebServiceWorkerNetworkProviderForFrame>(
      std::move(provider));
}

// static
std::unique_ptr<ServiceWorkerNetworkProvider>
ServiceWorkerNetworkProvider::CreateForSharedWorker(
    mojom::ServiceWorkerProviderInfoForSharedWorkerPtr info,
    network::mojom::URLLoaderFactoryAssociatedPtrInfo
        script_loader_factory_info,
    scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory) {
  // S13nServiceWorker: |info| holds info about the precreated provider host.
  if (info) {
    DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
    return base::WrapUnique(new ServiceWorkerNetworkProvider(
        std::move(info), std::move(script_loader_factory_info),
        std::move(fallback_loader_factory)));
  }

  return base::WrapUnique(new ServiceWorkerNetworkProvider(
      MSG_ROUTING_NONE,
      blink::mojom::ServiceWorkerProviderType::kForSharedWorker,
      GetNextProviderId(), true /* is_parent_frame_secure */,
      nullptr /* controller_service_worker */,
      std::move(fallback_loader_factory)));
}

// static
std::unique_ptr<ServiceWorkerNetworkProvider>
ServiceWorkerNetworkProvider::CreateForController(
    mojom::ServiceWorkerProviderInfoForStartWorkerPtr info) {
  return base::WrapUnique(new ServiceWorkerNetworkProvider(std::move(info)));
}

// static
ServiceWorkerNetworkProvider*
ServiceWorkerNetworkProvider::FromWebServiceWorkerNetworkProvider(
    blink::WebServiceWorkerNetworkProvider* provider) {
  if (!provider) {
    DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
    return nullptr;
  }
  return static_cast<WebServiceWorkerNetworkProviderForFrame*>(provider)
      ->provider();
}

ServiceWorkerNetworkProvider::~ServiceWorkerNetworkProvider() {
  if (context()) {
    context()->OnNetworkProviderDestroyed();
  }
}

int ServiceWorkerNetworkProvider::provider_id() const {
  if (!context())
    return kInvalidServiceWorkerProviderId;
  return context()->provider_id();
}

bool ServiceWorkerNetworkProvider::IsControlledByServiceWorker() const {
  return context() && context()->GetControllerVersionId() !=
                          blink::mojom::kInvalidServiceWorkerVersionId;
}

// Creates an invalid instance (provider_id() returns
// kInvalidServiceWorkerProviderId).
ServiceWorkerNetworkProvider::ServiceWorkerNetworkProvider() {}

// Constructor for service worker clients.
ServiceWorkerNetworkProvider::ServiceWorkerNetworkProvider(
    int route_id,
    blink::mojom::ServiceWorkerProviderType provider_type,
    int provider_id,
    bool is_parent_frame_secure,
    mojom::ControllerServiceWorkerInfoPtr controller_info,
    scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory) {
  DCHECK_NE(provider_id, kInvalidServiceWorkerProviderId);
  DCHECK(provider_type == blink::mojom::ServiceWorkerProviderType::kForWindow ||
         provider_type ==
             blink::mojom::ServiceWorkerProviderType::kForSharedWorker);

  ServiceWorkerProviderHostInfo host_info(provider_id, route_id, provider_type,
                                          is_parent_frame_secure);
  mojom::ServiceWorkerContainerAssociatedRequest client_request =
      mojo::MakeRequest(&host_info.client_ptr_info);
  mojom::ServiceWorkerContainerHostAssociatedPtrInfo host_ptr_info;
  host_info.host_request = mojo::MakeRequest(&host_ptr_info);
  DCHECK(host_info.host_request.is_pending());
  DCHECK(host_info.host_request.handle().is_valid());

  // current() may be null in tests.
  if (ChildThreadImpl::current()) {
    context_ = base::MakeRefCounted<ServiceWorkerProviderContext>(
        provider_id, provider_type, std::move(client_request),
        std::move(host_ptr_info), std::move(controller_info),
        std::move(fallback_loader_factory));
    ChildThreadImpl::current()->channel()->GetRemoteAssociatedInterface(
        &dispatcher_host_);
    dispatcher_host_->OnProviderCreated(std::move(host_info));
  } else {
    context_ = base::MakeRefCounted<ServiceWorkerProviderContext>(
        provider_id, provider_type, std::move(client_request),
        std::move(host_ptr_info), std::move(controller_info),
        std::move(fallback_loader_factory));
  }
}

// Constructor for precreated shared worker.
ServiceWorkerNetworkProvider::ServiceWorkerNetworkProvider(
    mojom::ServiceWorkerProviderInfoForSharedWorkerPtr info,
    network::mojom::URLLoaderFactoryAssociatedPtrInfo
        script_loader_factory_info,
    scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory) {
  context_ = base::MakeRefCounted<ServiceWorkerProviderContext>(
      info->provider_id,
      blink::mojom::ServiceWorkerProviderType::kForSharedWorker,
      std::move(info->client_request), std::move(info->host_ptr_info),
      nullptr /* controller */, std::move(fallback_loader_factory));
  if (script_loader_factory_info.is_valid())
    script_loader_factory_.Bind(std::move(script_loader_factory_info));
}

// Constructor for service worker execution contexts.
ServiceWorkerNetworkProvider::ServiceWorkerNetworkProvider(
    mojom::ServiceWorkerProviderInfoForStartWorkerPtr info) {
  context_ = base::MakeRefCounted<ServiceWorkerProviderContext>(
      info->provider_id, std::move(info->client_request),
      std::move(info->host_ptr_info));

  if (info->script_loader_factory_ptr_info.is_valid()) {
    script_loader_factory_.Bind(
        std::move(info->script_loader_factory_ptr_info));
  }
}

}  // namespace content
