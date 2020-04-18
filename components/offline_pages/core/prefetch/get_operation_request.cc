// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/get_operation_request.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_proto_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_request_fetcher.h"
#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace offline_pages {

GetOperationRequest::GetOperationRequest(
    const std::string& name,
    version_info::Channel channel,
    net::URLRequestContextGetter* request_context_getter,
    const PrefetchRequestFinishedCallback& callback)
    : callback_(callback) {
  fetcher_ = PrefetchRequestFetcher::CreateForGet(
      GetOperationRequestURL(name, channel), request_context_getter,
      base::Bind(&GetOperationRequest::OnCompleted,
                 // Fetcher is owned by this instance.
                 base::Unretained(this), name));
}

GetOperationRequest::~GetOperationRequest() {}

void GetOperationRequest::OnCompleted(
    const std::string& assigned_operation_name,
    PrefetchRequestStatus status,
    const std::string& data) {
  if (status != PrefetchRequestStatus::SUCCESS) {
    callback_.Run(status, assigned_operation_name,
                  std::vector<RenderPageInfo>());
    return;
  }

  std::vector<RenderPageInfo> pages;
  std::string found_operation_name = ParseOperationResponse(data, &pages);
  if (found_operation_name.empty()) {
    callback_.Run(PrefetchRequestStatus::SHOULD_RETRY_WITH_BACKOFF,
                  assigned_operation_name, std::vector<RenderPageInfo>());
    return;
  }

  callback_.Run(PrefetchRequestStatus::SUCCESS, assigned_operation_name, pages);
}

}  // offline_pages
