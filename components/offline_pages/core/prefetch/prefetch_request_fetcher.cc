// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_request_fetcher.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"
#include "google_apis/google_api_keys.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

// Content type needed in order to communicate with the server in binary
// proto format.
const char kRequestContentType[] = "application/x-protobuf";

}  // namespace

// static
std::unique_ptr<PrefetchRequestFetcher> PrefetchRequestFetcher::CreateForGet(
    const GURL& url,
    net::URLRequestContextGetter* request_context_getter,
    const FinishedCallback& callback) {
  return base::WrapUnique(new PrefetchRequestFetcher(
      url, std::string(), request_context_getter, callback));
}

// static
std::unique_ptr<PrefetchRequestFetcher> PrefetchRequestFetcher::CreateForPost(
    const GURL& url,
    const std::string& message,
    net::URLRequestContextGetter* request_context_getter,
    const FinishedCallback& callback) {
  return base::WrapUnique(new PrefetchRequestFetcher(
      url, message, request_context_getter, callback));
}

PrefetchRequestFetcher::PrefetchRequestFetcher(
    const GURL& url,
    const std::string& message,
    net::URLRequestContextGetter* request_context_getter,
    const FinishedCallback& callback)
    : request_context_getter_(request_context_getter), callback_(callback) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("offline_prefetch", R"(
        semantics {
          sender: "Offline Prefetch"
          description:
            "Chromium interacts with Offline Page Service to prefetch "
            "suggested website resources."
          trigger:
            "When there are suggested website resources to fetch."
          data:
            "URLs of the suggested website resources to fetch."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can enable or disable the offline prefetch by toggling"
            "chrome://flags#offline-prefetch in Chromium on Android."
          policy_exception_justification:
            "Not implemented, considered not useful."
        })");
  url_fetcher_ = net::URLFetcher::Create(
      url, message.empty() ? net::URLFetcher::GET : net::URLFetcher::POST, this,
      traffic_annotation);
  url_fetcher_->SetRequestContext(request_context_getter_.get());
  url_fetcher_->SetAutomaticallyRetryOn5xx(false);
  url_fetcher_->SetAutomaticallyRetryOnNetworkChanges(0);
  std::string experiment_header = PrefetchExperimentHeader();
  if (!experiment_header.empty())
    url_fetcher_->AddExtraRequestHeader(experiment_header);
  if (message.empty()) {
    std::string content_type_header(net::HttpRequestHeaders::kContentType);
    content_type_header += ": ";
    content_type_header += kRequestContentType;
    url_fetcher_->AddExtraRequestHeader(content_type_header);
  } else {
    url_fetcher_->SetUploadData(kRequestContentType, message);
  }
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES);
  url_fetcher_->Start();
}

PrefetchRequestFetcher::~PrefetchRequestFetcher() {}

void PrefetchRequestFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  std::string data;
  PrefetchRequestStatus status = ParseResponse(source, &data);

  // TODO(jianli): Report UMA.

  callback_.Run(status, data);
}

PrefetchRequestStatus PrefetchRequestFetcher::ParseResponse(
    const net::URLFetcher* source,
    std::string* data) {
  if (!source->GetStatus().is_success()) {
    net::Error net_error = source->GetStatus().ToNetError();
    DVLOG(1) << "Net error: " << net_error;
    return (net_error == net::ERR_BLOCKED_BY_ADMINISTRATOR)
               ? PrefetchRequestStatus::SHOULD_SUSPEND
               : PrefetchRequestStatus::SHOULD_RETRY_WITHOUT_BACKOFF;
  }

  net::HttpStatusCode response_status =
      static_cast<net::HttpStatusCode>(source->GetResponseCode());
  if (response_status != net::HTTP_OK) {
    DVLOG(1) << "HTTP status: " << response_status;
    return (response_status == net::HTTP_NOT_IMPLEMENTED)
               ? PrefetchRequestStatus::SHOULD_SUSPEND
               : PrefetchRequestStatus::SHOULD_RETRY_WITH_BACKOFF;
  }

  if (!source->GetResponseAsString(data) || data->empty()) {
    DVLOG(1) << "Failed to get response or empty response";
    return PrefetchRequestStatus::SHOULD_RETRY_WITH_BACKOFF;
  }

  return PrefetchRequestStatus::SUCCESS;
}

}  // offline_pages
