// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/cors/cors_url_loader.h"

#include "base/stl_util.h"
#include "base/strings/pattern.h"
#include "services/network/cors/preflight_controller.h"
#include "services/network/public/cpp/cors/cors.h"
#include "services/network/public/cpp/cors/cors_legacy.h"
#include "url/url_util.h"

namespace network {

namespace cors {

namespace {

bool IsOriginWhiteListedTrustworthy(const url::Origin& origin) {
  if (origin.unique())
    return false;

  // Note: NoAccessSchemes are managed by per-process basis. This check is for
  // use of network service disabled.
  if (base::ContainsValue(url::GetNoAccessSchemes(), origin.scheme()))
    return false;

  if (base::ContainsValue(legacy::GetSecureOrigins(), origin.Serialize()))
    return true;

  for (const auto& origin_or_pattern : legacy::GetSecureOrigins()) {
    if (base::MatchPattern(origin.host(), origin_or_pattern))
      return true;
  }
  return false;
}

bool CalculateCORSFlag(const ResourceRequest& request) {
  if (request.fetch_request_mode == mojom::FetchRequestMode::kNavigate)
    return false;
  url::Origin url_origin = url::Origin::Create(request.url);
  if (IsOriginWhiteListedTrustworthy(url_origin))
    return false;
  if (!request.request_initiator.has_value())
    return true;
  url::Origin security_origin(request.request_initiator.value());
  return !security_origin.IsSameOriginWith(url_origin);
}

base::Optional<std::string> GetHeaderString(
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& header_name) {
  std::string header_value;
  if (!headers->GetNormalizedHeader(header_name, &header_value))
    return base::nullopt;
  return header_value;
}

bool NeedsPreflight(const ResourceRequest& request) {
  if (request.is_external_request)
    return true;

  if (request.fetch_request_mode ==
      mojom::FetchRequestMode::kCORSWithForcedPreflight) {
    return true;
  }

  if (!IsCORSSafelistedMethod(request.method))
    return true;

  for (const auto& header : request.headers.GetHeaderVector()) {
    if (!IsCORSSafelistedHeader(header.key, header.value) &&
        !IsForbiddenHeader(header.key)) {
      return true;
    }
  }
  return false;
}

}  // namespace

// TODO(toyoshim): This class still lacks right CORS checks in redirects.
// See http://crbug/736308 to track the progress.
CORSURLLoader::CORSURLLoader(
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojom::URLLoaderFactory* network_loader_factory,
    const base::RepeatingCallback<void(int)>& preflight_finalizer)
    : network_loader_factory_(network_loader_factory),
      network_client_binding_(this),
      request_(resource_request),
      forwarding_client_(std::move(client)),
      last_response_url_(resource_request.url),
      fetch_cors_flag_(CalculateCORSFlag(resource_request)),
      weak_factory_(this) {
  DCHECK(network_loader_factory_);
  DCHECK(resource_request.request_initiator);

  if (fetch_cors_flag_ &&
      request_.fetch_request_mode == mojom::FetchRequestMode::kSameOrigin) {
    forwarding_client_->OnComplete(URLLoaderCompletionStatus(
        CORSErrorStatus(mojom::CORSError::kDisallowedByMode)));
    forwarding_client_.reset();
    return;
  }

  if (fetch_cors_flag_ &&
      cors::IsCORSEnabledRequestMode(request_.fetch_request_mode)) {
    // Username and password should be stripped in a CORS-enabled request.
    if (request_.url.has_username() || request_.url.has_password()) {
      GURL::Replacements replacements;
      replacements.SetUsernameStr("");
      replacements.SetPasswordStr("");
      request_.url = request_.url.ReplaceComponents(replacements);
      last_response_url_ = request_.url;
    }
  }

  if (fetch_cors_flag_) {
    request_.headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                               request_.request_initiator->Serialize());
  }

  if (!fetch_cors_flag_ || !NeedsPreflight(request_)) {
    StartNetworkRequest(routing_id, request_id, options, traffic_annotation,
                        base::nullopt);
    return;
  }

  base::OnceCallback<void()> preflight_finalizer_for_request;
  if (preflight_finalizer) {
    preflight_finalizer_for_request =
        base::BindOnce(preflight_finalizer, request_id);
  }

  PreflightController::GetDefaultController()->PerformPreflightCheck(
      base::BindOnce(&CORSURLLoader::StartNetworkRequest,
                     weak_factory_.GetWeakPtr(), routing_id, request_id,
                     options, traffic_annotation),
      request_id, request_,
      net::NetworkTrafficAnnotationTag(traffic_annotation),
      network_loader_factory, std::move(preflight_finalizer_for_request));
}

CORSURLLoader::~CORSURLLoader() {}

void CORSURLLoader::FollowRedirect(
    const base::Optional<net::HttpRequestHeaders>& modified_request_headers) {
  DCHECK(!modified_request_headers.has_value()) << "Redirect with modified "
                                                   "headers was not supported "
                                                   "yet. crbug.com/845683";
  DCHECK(network_loader_);
  DCHECK(is_waiting_follow_redirect_call_);
  is_waiting_follow_redirect_call_ = false;
  network_loader_->FollowRedirect(base::nullopt);
}

void CORSURLLoader::ProceedWithResponse() {
  NOTREACHED();
}

void CORSURLLoader::SetPriority(net::RequestPriority priority,
                                int32_t intra_priority_value) {
  if (network_loader_)
    network_loader_->SetPriority(priority, intra_priority_value);
}

void CORSURLLoader::PauseReadingBodyFromNet() {
  DCHECK(!is_waiting_follow_redirect_call_);
  if (network_loader_)
    network_loader_->PauseReadingBodyFromNet();
}

void CORSURLLoader::ResumeReadingBodyFromNet() {
  DCHECK(!is_waiting_follow_redirect_call_);
  if (network_loader_)
    network_loader_->ResumeReadingBodyFromNet();
}

void CORSURLLoader::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    mojom::DownloadedTempFilePtr downloaded_file) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  if (fetch_cors_flag_ &&
      IsCORSEnabledRequestMode(request_.fetch_request_mode)) {
    // TODO(toyoshim): Reflect --allow-file-access-from-files flag.
    base::Optional<mojom::CORSError> cors_error = CheckAccess(
        last_response_url_, response_head.headers->response_code(),
        GetHeaderString(response_head.headers,
                        header_names::kAccessControlAllowOrigin),
        GetHeaderString(response_head.headers,
                        header_names::kAccessControlAllowCredentials),
        request_.fetch_credentials_mode, *request_.request_initiator);
    if (cors_error) {
      // TODO(toyoshim): Generate related_response_headers here.
      CORSErrorStatus cors_error_status(*cors_error);
      HandleComplete(URLLoaderCompletionStatus(cors_error_status));
      return;
    }
  }
  forwarding_client_->OnReceiveResponse(response_head,
                                        std::move(downloaded_file));
}

void CORSURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);

  // TODO(toyoshim): Following code expects OnReceivedRedirect is invoked
  // asynchronously, and |last_response_url_| and other methods should not be
  // accessed until FollowRedirect() is called.
  // We need to ensure callback behaviors once redirect implementation in this
  // class is ready for testing.
  is_waiting_follow_redirect_call_ = true;
  last_response_url_ = redirect_info.new_url;
  forwarding_client_->OnReceiveRedirect(redirect_info, response_head);
}

void CORSURLLoader::OnDataDownloaded(int64_t data_len,
                                     int64_t encoded_data_len) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  forwarding_client_->OnDataDownloaded(data_len, encoded_data_len);
}

void CORSURLLoader::OnUploadProgress(int64_t current_position,
                                     int64_t total_size,
                                     OnUploadProgressCallback ack_callback) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  forwarding_client_->OnUploadProgress(current_position, total_size,
                                       std::move(ack_callback));
}

void CORSURLLoader::OnReceiveCachedMetadata(const std::vector<uint8_t>& data) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  forwarding_client_->OnReceiveCachedMetadata(data);
}

void CORSURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  forwarding_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void CORSURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  forwarding_client_->OnStartLoadingResponseBody(std::move(body));
}

void CORSURLLoader::OnComplete(const URLLoaderCompletionStatus& status) {
  DCHECK(network_loader_);
  DCHECK(forwarding_client_);
  DCHECK(!is_waiting_follow_redirect_call_);
  HandleComplete(status);
}

void CORSURLLoader::StartNetworkRequest(
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    base::Optional<CORSErrorStatus> status) {
  if (status) {
    forwarding_client_->OnComplete(URLLoaderCompletionStatus(*status));
    forwarding_client_.reset();
    return;
  }

  mojom::URLLoaderClientPtr network_client;
  network_client_binding_.Bind(mojo::MakeRequest(&network_client));
  // Binding |this| as an unretained pointer is safe because
  // |network_client_binding_| shares this object's lifetime.
  network_client_binding_.set_connection_error_handler(base::BindOnce(
      &CORSURLLoader::OnUpstreamConnectionError, base::Unretained(this)));
  network_loader_factory_->CreateLoaderAndStart(
      mojo::MakeRequest(&network_loader_), routing_id, request_id, options,
      request_, std::move(network_client), traffic_annotation);
}

void CORSURLLoader::OnUpstreamConnectionError() {
  // |network_client_binding_| has experienced a connection error and will no
  // longer call any of the mojom::URLLoaderClient methods above. The client
  // pipe to the downstream client is closed to inform it of this failure. The
  // client should respond by closing its mojom::URLLoader pipe which will cause
  // this object to be destroyed.
  forwarding_client_.reset();
}

void CORSURLLoader::HandleComplete(const URLLoaderCompletionStatus& status) {
  forwarding_client_->OnComplete(status);
  forwarding_client_.reset();

  // Close pipes to ignore possible subsequent callback invocations.
  network_client_binding_.Close();
  network_loader_.reset();
}

}  // namespace cors

}  // namespace network
