// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_FETCH_DISPATCHER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_FETCH_DISPATCHER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/browser/service_worker/service_worker_metrics.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_type.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/log/net_log_with_source.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "third_party/blink/public/mojom/blob/blob.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

class ServiceWorkerVersion;
class URLLoaderFactoryGetter;

// A helper class to dispatch fetch event to a service worker.
class CONTENT_EXPORT ServiceWorkerFetchDispatcher {
 public:
  // Indicates how the service worker handled a fetch event.
  enum class FetchEventResult {
    // Browser should fallback to native fetch.
    kShouldFallback,
    // Service worker provided a ServiceWorkerResponse.
    kGotResponse
  };

  using FetchCallback =
      base::OnceCallback<void(ServiceWorkerStatusCode,
                              FetchEventResult,
                              const ServiceWorkerResponse&,
                              blink::mojom::ServiceWorkerStreamHandlePtr,
                              blink::mojom::BlobPtr,
                              scoped_refptr<ServiceWorkerVersion>)>;

  // |request_body_*| and |client_id| are used in non-S13nServiceWorker only.
  ServiceWorkerFetchDispatcher(
      std::unique_ptr<network::ResourceRequest> request,
      const std::string& request_body_blob_uuid,
      uint64_t request_body_blob_size,
      blink::mojom::BlobPtr request_body_blob,
      const std::string& client_id,
      scoped_refptr<ServiceWorkerVersion> version,
      const net::NetLogWithSource& net_log,
      base::OnceClosure prepare_callback,
      FetchCallback fetch_callback);
  ~ServiceWorkerFetchDispatcher();

  // If appropriate, starts the navigation preload request and creates
  // |preload_handle_|. Returns true if it started navigation preload.
  // |on_response| is invoked in OnReceiveResponse().
  bool MaybeStartNavigationPreload(net::URLRequest* original_request,
                                   base::OnceClosure on_response);
  // S13nServiceWorker
  // Same as above but for S13N.
  bool MaybeStartNavigationPreloadWithURLLoader(
      const network::ResourceRequest& original_request,
      URLLoaderFactoryGetter* url_loader_factory_getter,
      base::OnceClosure on_response);

  // Dispatches a fetch event to the |version| given in ctor, and fires
  // |fetch_callback| (also given in ctor) when finishes. It runs
  // |prepare_callback| as an intermediate step once the version is activated
  // and running.
  void Run();

 private:
  class ResponseCallback;
  class URLLoaderAssets;

  void DidWaitForActivation();
  void StartWorker();
  void DidStartWorker(ServiceWorkerStatusCode status);
  void DispatchFetchEvent();
  void DidFailToDispatch(std::unique_ptr<ResponseCallback> callback,
                         ServiceWorkerStatusCode status);
  void DidFail(ServiceWorkerStatusCode status);
  void DidFinish(int request_id,
                 FetchEventResult fetch_result,
                 const ServiceWorkerResponse& response,
                 blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
                 blink::mojom::BlobPtr body_as_blob);
  void Complete(ServiceWorkerStatusCode status,
                FetchEventResult fetch_result,
                const ServiceWorkerResponse& response,
                blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
                blink::mojom::BlobPtr body_as_blob);

  static void OnFetchEventFinished(
      ServiceWorkerVersion* version,
      int event_finish_id,
      scoped_refptr<URLLoaderAssets> url_loader_assets,
      blink::mojom::ServiceWorkerEventStatus status,
      base::Time dispatch_event_time);

  ServiceWorkerMetrics::EventType GetEventType() const;

  std::unique_ptr<network::ResourceRequest> request_;

  // Non-S13nServiceWorker uses these. ///////////////////////////////
  std::string request_body_blob_uuid_;
  uint64_t request_body_blob_size_ = 0;
  blink::mojom::BlobPtr request_body_blob_;
  std::string client_id_;
  ///////////////////////////////////////////////////////////////////

  scoped_refptr<ServiceWorkerVersion> version_;
  ResourceType resource_type_;
  net::NetLogWithSource net_log_;
  base::OnceClosure prepare_callback_;
  FetchCallback fetch_callback_;
  bool did_complete_;

  scoped_refptr<URLLoaderAssets> url_loader_assets_;

  // |preload_handle_| holds the URLLoader and URLLoaderClient for the service
  // worker to receive the navigation preload response. It's passed to the
  // service worker along with the fetch event.
  mojom::FetchEventPreloadHandlePtr preload_handle_;

  base::WeakPtrFactory<ServiceWorkerFetchDispatcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerFetchDispatcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_FETCH_DISPATCHER_H_
