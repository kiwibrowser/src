// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_proxying_url_loader_factory.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "net/http/http_util.h"

namespace extensions {

WebRequestProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    WebRequestProxyingURLLoaderFactory* factory,
    uint64_t request_id,
    int32_t network_service_request_id,
    int32_t routing_id,
    uint32_t options,
    const network::ResourceRequest& request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderRequest loader_request,
    network::mojom::URLLoaderClientPtr client)
    : factory_(factory),
      request_(request),
      request_id_(request_id),
      network_service_request_id_(network_service_request_id),
      routing_id_(routing_id),
      options_(options),
      traffic_annotation_(traffic_annotation),
      proxied_loader_binding_(this, std::move(loader_request)),
      target_client_(std::move(client)),
      proxied_client_binding_(this),
      weak_factory_(this) {}

WebRequestProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {
  // This is important to ensure that no outstanding blocking requests continue
  // to reference state owned by this object.
  if (info_) {
    ExtensionWebRequestEventRouter::GetInstance()->OnRequestWillBeDestroyed(
        factory_->browser_context_, &info_.value());
  }
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::Restart() {
  // Derive a new WebRequestInfo value any time |Restart()| is called, because
  // the details in |request_| may have changed e.g. if we've been redirected.
  info_.emplace(
      request_id_, factory_->render_process_id_, factory_->render_frame_id_,
      factory_->navigation_ui_data_ ? factory_->navigation_ui_data_->DeepCopy()
                                    : nullptr,
      routing_id_, factory_->resource_context_, request_);

  auto continuation =
      base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                          weak_factory_.GetWeakPtr());
  redirect_url_ = GURL();
  int result = ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRequest(
      factory_->browser_context_, factory_->info_map_, &info_.value(),
      continuation, &redirect_url_);
  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    OnRequestError(network::URLLoaderCompletionStatus(result));
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_binding_.is_bound())
      proxied_client_binding_.PauseIncomingMethodCallProcessing();
    return;
  }
  DCHECK_EQ(net::OK, result);

  ContinueToBeforeSendHeaders(net::OK);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const base::Optional<net::HttpRequestHeaders>& modified_request_headers) {
  DCHECK(!modified_request_headers.has_value()) << "Redirect with modified "
                                                   "headers was not supported "
                                                   "yet. crbug.com/845683";
  if (ignore_next_follow_redirect_) {
    ignore_next_follow_redirect_ = false;
    return;
  }

  if (target_loader_.is_bound())
    target_loader_->FollowRedirect(base::nullopt);
  Restart();
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ProceedWithResponse() {
  if (target_loader_.is_bound())
    target_loader_->ProceedWithResponse();
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound())
    target_loader_->SetPriority(priority, intra_priority_value);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    PauseReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->PauseReadingBodyFromNet();
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->ResumeReadingBodyFromNet();
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    const network::ResourceResponseHead& head,
    network::mojom::DownloadedTempFilePtr downloaded_file) {
  current_response_ = head;
  HandleResponseOrRedirectHeaders(base::BindRepeating(
      &InProgressRequest::ContinueToResponseStarted, weak_factory_.GetWeakPtr(),
      base::Passed(&downloaded_file)));
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {
  current_response_ = head;
  HandleResponseOrRedirectHeaders(
      base::BindRepeating(&InProgressRequest::ContinueToBeforeRedirect,
                          weak_factory_.GetWeakPtr(), redirect_info));
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnDataDownloaded(
    int64_t data_length,
    int64_t encoded_length) {
  target_client_->OnDataDownloaded(data_length, encoded_length);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    OnReceiveCachedMetadata(const std::vector<uint8_t>& data) {
  target_client_->OnReceiveCachedMetadata(data);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    OnTransferSizeUpdated(int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    OnStartLoadingResponseBody(mojo::ScopedDataPipeConsumerHandle body) {
  target_client_->OnStartLoadingResponseBody(std::move(body));
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  if (status.error_code != net::OK) {
    OnRequestError(status);
    return;
  }

  target_client_->OnComplete(status);
  ExtensionWebRequestEventRouter::GetInstance()->OnCompleted(
      factory_->browser_context_, factory_->info_map_, &info_.value(),
      status.error_code);

  // Deletes |this|.
  factory_->RemoveRequest(request_id_);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeSendHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (!redirect_url_.is_empty()) {
    constexpr int kInternalRedirectStatusCode = 307;

    net::RedirectInfo redirect_info;
    redirect_info.status_code = kInternalRedirectStatusCode;
    redirect_info.new_method = request_.method;
    redirect_info.new_url = redirect_url_;
    redirect_info.new_site_for_cookies = redirect_url_;

    network::ResourceResponseHead head;
    std::string headers = base::StringPrintf(
        "HTTP/1.1 %i Internal Redirect\n"
        "Location: %s\n"
        "Non-Authoritative-Reason: WebRequest API\n\n",
        kInternalRedirectStatusCode, redirect_url_.spec().c_str());
    head.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.length()));
    head.encoded_data_length = 0;

    current_response_ = head;
    ContinueToBeforeRedirect(redirect_info, net::OK);
    return;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    // NOTE: While it does not appear to be documented (and in fact it may be
    // intuitive), |onBeforeSendHeaders| is only dispatched for HTTP and HTTPS
    // requests.

    auto continuation = base::BindRepeating(
        &InProgressRequest::ContinueToSendHeaders, weak_factory_.GetWeakPtr());
    int result =
        ExtensionWebRequestEventRouter::GetInstance()->OnBeforeSendHeaders(
            factory_->browser_context_, factory_->info_map_, &info_.value(),
            continuation, &request_.headers);

    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      // The request was cancelled synchronously. Dispatch an error notification
      // and terminate the request.
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      if (proxied_client_binding_.is_bound())
        proxied_client_binding_.PauseIncomingMethodCallProcessing();
      return;
    }
    DCHECK_EQ(net::OK, result);
  }

  ContinueToSendHeaders(net::OK);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ContinueToSendHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    // NOTE: While it does not appear to be documented (and in fact it may be
    // intuitive), |onSendHeaders| is only dispatched for HTTP and HTTPS
    // requests.
    ExtensionWebRequestEventRouter::GetInstance()->OnSendHeaders(
        factory_->browser_context_, factory_->info_map_, &info_.value(),
        request_.headers);
  }

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // No extensions have cancelled us up to this point, so it's now OK to
    // initiate the real network request.
    network::mojom::URLLoaderClientPtr proxied_client;
    proxied_client_binding_.Bind(mojo::MakeRequest(&proxied_client));
    factory_->target_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&target_loader_), info_->routing_id,
        network_service_request_id_, options_, request_,
        std::move(proxied_client), traffic_annotation_);
  }

  // From here the lifecycle of this request is driven by subsequent events on
  // either |proxy_loader_binding_| or |proxy_client_binding_|.
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ContinueToResponseStarted(
        network::mojom::DownloadedTempFilePtr downloaded_file,
        int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  std::string redirect_location;
  if (override_headers_ && override_headers_->IsRedirect(&redirect_location)) {
    // The response headers may have been overridden by an |onHeadersReceived|
    // handler and may have been changed to a redirect. We handle that here
    // instead of acting like regular request completion.
    //
    // Note that we can't actually change how the Network Service handles the
    // original request at this point, so our "redirect" is really just
    // generating an artificial |onBeforeRedirect| event and starting a new
    // request to the Network Service. Our client shouldn't know the difference.
    GURL new_url(redirect_location);

    net::RedirectInfo redirect_info;
    redirect_info.status_code = override_headers_->response_code();
    redirect_info.new_method = request_.method;
    redirect_info.new_url = new_url;
    redirect_info.new_site_for_cookies = new_url;

    current_response_.headers = override_headers_;

    // These will get re-bound when a new request is initiated after Restart()
    // below.
    proxied_client_binding_.Close();
    target_loader_.reset();

    // The client will send a |FollowRedirect()| in response to the impending
    // |OnReceiveRedirect()| we send it. We don't want that to get forwarded to
    // the backing URLLoader since it knows nothing about any such redirect and
    // would have no idea how to comply.
    ignore_next_follow_redirect_ = true;

    ContinueToBeforeRedirect(redirect_info, net::OK);
    Restart();
    return;
  }

  info_->AddResponseInfoFromResourceResponse(current_response_);

  proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  ExtensionWebRequestEventRouter::GetInstance()->OnResponseStarted(
      factory_->browser_context_, factory_->info_map_, &info_.value(), net::OK);
  target_client_->OnReceiveResponse(current_response_,
                                    std::move(downloaded_file));
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    ContinueToBeforeRedirect(const net::RedirectInfo& redirect_info,
                             int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  info_->AddResponseInfoFromResourceResponse(current_response_);

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRedirect(
      factory_->browser_context_, factory_->info_map_, &info_.value(),
      redirect_info.new_url);
  target_client_->OnReceiveRedirect(redirect_info, current_response_);
  request_.url = redirect_info.new_url;
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::
    HandleResponseOrRedirectHeaders(
        const net::CompletionCallback& continuation) {
  override_headers_ = nullptr;
  if (request_.url.SchemeIsHTTPOrHTTPS()) {
    int result =
        ExtensionWebRequestEventRouter::GetInstance()->OnHeadersReceived(
            factory_->browser_context_, factory_->info_map_, &info_.value(),
            continuation, current_response_.headers.get(), &override_headers_,
            &allowed_unsafe_redirect_url_);
    if (result == net::ERR_BLOCKED_BY_CLIENT) {
      OnRequestError(network::URLLoaderCompletionStatus(result));
      return;
    }

    if (result == net::ERR_IO_PENDING) {
      // One or more listeners is blocking, so the request must be paused until
      // they respond. |continuation| above will be invoked asynchronously to
      // continue or cancel the request.
      //
      // We pause the binding here to prevent further client message processing.
      proxied_client_binding_.PauseIncomingMethodCallProcessing();
      return;
    }

    DCHECK_EQ(net::OK, result);
  }

  continuation.Run(net::OK);
}

void WebRequestProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  target_client_->OnComplete(status);
  ExtensionWebRequestEventRouter::GetInstance()->OnErrorOccurred(
      factory_->browser_context_, factory_->info_map_, &info_.value(),
      true /* started */, status.error_code);

  // Deletes |this|.
  factory_->RemoveRequest(request_id_);
}

WebRequestProxyingURLLoaderFactory::WebRequestProxyingURLLoaderFactory(
    void* browser_context,
    content::ResourceContext* resource_context,
    InfoMap* info_map)
    : RefCountedDeleteOnSequence(content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO)),
      browser_context_(browser_context),
      resource_context_(resource_context),
      info_map_(info_map) {}

void WebRequestProxyingURLLoaderFactory::StartProxying(
    int render_process_id,
    int render_frame_id,
    std::unique_ptr<ExtensionNavigationUIData> navigation_ui_data,
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory_info,
    base::OnceClosure on_disconnect) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  on_disconnect_ = std::move(on_disconnect);
  render_process_id_ = render_process_id;
  render_frame_id_ = render_frame_id;
  navigation_ui_data_ = std::move(navigation_ui_data);

  target_factory_.Bind(std::move(target_factory_info));
  target_factory_.set_connection_error_handler(
      base::BindOnce(&WebRequestProxyingURLLoaderFactory::OnTargetFactoryError,
                     base::Unretained(this)));

  proxy_bindings_.AddBinding(this, std::move(loader_request));
  proxy_bindings_.set_connection_error_handler(base::BindRepeating(
      &WebRequestProxyingURLLoaderFactory::OnProxyBindingError,
      base::Unretained(this)));
}

void WebRequestProxyingURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // The request ID doesn't really matter in the Network Service path. It just
  // needs to be unique per-BrowserContext so extensions can make sense of it.
  // Note that |network_service_request_id_| by contrast is not necessarily
  // unique, so we don't use it for identity here.
  const uint64_t web_request_id = next_request_id_++;
  auto result = requests_.emplace(
      web_request_id,
      std::make_unique<InProgressRequest>(
          this, web_request_id, request_id, routing_id, options, request,
          traffic_annotation, std::move(loader_request), std::move(client)));
  result.first->second->Restart();
}

void WebRequestProxyingURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest loader_request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  proxy_bindings_.AddBinding(this, std::move(loader_request));
}

WebRequestProxyingURLLoaderFactory::~WebRequestProxyingURLLoaderFactory() =
    default;

void WebRequestProxyingURLLoaderFactory::OnTargetFactoryError() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  target_factory_.reset();
  if (proxy_bindings_.empty()) {
    // Deletes |this|.
    std::move(on_disconnect_).Run();
  }
}

void WebRequestProxyingURLLoaderFactory::OnProxyBindingError() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (proxy_bindings_.empty() && !target_factory_.is_bound()) {
    // Deletes |this|.
    std::move(on_disconnect_).Run();
  }
}

void WebRequestProxyingURLLoaderFactory::RemoveRequest(uint64_t request_id) {
  auto it = requests_.find(request_id);
  DCHECK(it != requests_.end());
  requests_.erase(it);
}

}  // namespace extensions
