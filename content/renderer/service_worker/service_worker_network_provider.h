// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_NETWORK_PROVIDER_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_NETWORK_PROVIDER_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/controller_service_worker.mojom.h"
#include "content/common/service_worker/service_worker.mojom.h"
#include "content/common/service_worker/service_worker_provider.mojom.h"
#include "content/renderer/service_worker/service_worker_provider_context.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_provider_type.mojom.h"

namespace blink {
class WebLocalFrame;
class WebServiceWorkerNetworkProvider;
}  // namespace blink

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace content {

namespace mojom {
class URLLoaderFactory;
}

struct RequestNavigationParams;
class ServiceWorkerProviderContext;

// ServiceWorkerNetworkProvider enables the browser process to recognize
// resource requests from Blink that should be handled by service worker
// machinery rather than the usual network stack.
//
// All requests associated with a network provider are tagged with its
// |provider_id| (from ServiceWorkerProviderContext). The browser
// process can then route the request to this provider's corresponding
// ServiceWorkerProviderHost.
//
// It is created for both service worker clients and execution contexts. It is
// instantiated prior to the main resource load being started and remains
// allocated until after the last subresource load has occurred. It is owned by
// the appropriate DocumentLoader for the provider (i.e., the loader for a
// document, or the shadow page's loader for a shared worker or service worker).
// Each request coming from the DocumentLoader is tagged with the provider_id in
// WillSendRequest.
class CONTENT_EXPORT ServiceWorkerNetworkProvider {
 public:
  // Creates a ServiceWorkerNetworkProvider for navigation and wraps it
  // with WebServiceWorkerNetworkProvider to be owned by Blink.
  //
  // For S13nServiceWorker:
  // |controller_info| contains the endpoint and object info that is needed to
  // set up the controller service worker for the client.
  // |fallback_loader_factory| is a default loader factory for fallback
  // requests, and is used when we create a subresource loader for controllees.
  // This is non-null only if the provider is created for controllees, and if
  // the loading context, e.g. a frame, provides it.
  static std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
  CreateForNavigation(
      int route_id,
      const RequestNavigationParams& request_params,
      blink::WebLocalFrame* frame,
      bool content_initiated,
      mojom::ControllerServiceWorkerInfoPtr controller_info,
      scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory);

  // Creates a ServiceWorkerNetworkProvider for a shared worker (as a
  // non-document service worker client).
  static std::unique_ptr<ServiceWorkerNetworkProvider> CreateForSharedWorker(
      mojom::ServiceWorkerProviderInfoForSharedWorkerPtr info,
      network::mojom::URLLoaderFactoryAssociatedPtrInfo
          script_loader_factory_info,
      scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory);

  // Creates a ServiceWorkerNetworkProvider for a "controller" (i.e.
  // a service worker execution context).
  static std::unique_ptr<ServiceWorkerNetworkProvider> CreateForController(
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr info);

  // Valid only for WebServiceWorkerNetworkProvider created by
  // CreateForNavigation.
  static ServiceWorkerNetworkProvider* FromWebServiceWorkerNetworkProvider(
      blink::WebServiceWorkerNetworkProvider*);

  ~ServiceWorkerNetworkProvider();

  int provider_id() const;
  ServiceWorkerProviderContext* context() const { return context_.get(); }

  network::mojom::URLLoaderFactory* script_loader_factory() {
    return script_loader_factory_.get();
  }

  bool IsControlledByServiceWorker() const;

 private:
  // Creates an invalid instance (provider_id() returns
  // kInvalidServiceWorkerProviderId).
  ServiceWorkerNetworkProvider();

  // This is for service worker clients (i.e., |type| must be kForWindow or
  // kForSharedWorker). |is_parent_frame_secure| is only relevant when the
  // |type| is kForWindow.
  //
  // For S13nServiceWorker:
  // See the comment at CreateForNavigation() for |controller_info| and
  // |fallback_loader_factory|.
  ServiceWorkerNetworkProvider(
      int route_id,
      blink::mojom::ServiceWorkerProviderType type,
      int provider_id,
      bool is_parent_frame_secure,
      mojom::ControllerServiceWorkerInfoPtr controller_info,
      scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory);

  ServiceWorkerNetworkProvider(
      mojom::ServiceWorkerProviderInfoForSharedWorkerPtr info,
      network::mojom::URLLoaderFactoryAssociatedPtrInfo
          script_loader_factory_info,
      scoped_refptr<network::SharedURLLoaderFactory> fallback_loader_factory);

  // This is for controllers, used in CreateForController.
  explicit ServiceWorkerNetworkProvider(
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr info);

  scoped_refptr<ServiceWorkerProviderContext> context_;
  mojom::ServiceWorkerDispatcherHostAssociatedPtr dispatcher_host_;

  // The URL loader factory for loading worker scripts, used for service workers
  // and shared workers.
  network::mojom::URLLoaderFactoryAssociatedPtr script_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerNetworkProvider);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_NETWORK_PROVIDER_H_
