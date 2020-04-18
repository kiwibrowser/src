// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/url_fetcher_block_adapter.h"

#include "base/logging.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

URLFetcherBlockAdapter::URLFetcherBlockAdapter(
    const GURL& url,
    net::URLRequestContextGetter* request_context,
    web::URLFetcherBlockAdapterCompletion completion_handler)
    : url_(url),
      request_context_(request_context),
      completion_handler_([completion_handler copy]) {
}

URLFetcherBlockAdapter::~URLFetcherBlockAdapter() {
}

void URLFetcherBlockAdapter::Start() {
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
  fetcher_->SetRequestContext(request_context_);
  fetcher_->Start();
}

void URLFetcherBlockAdapter::OnURLFetchComplete(const net::URLFetcher* source) {
  std::string response;
  if (!source->GetResponseAsString(&response)) {
    DLOG(WARNING) << "String for resource URL not found" << source->GetURL();
  }
  NSData* data =
      [NSData dataWithBytes:response.c_str() length:response.length()];
  completion_handler_.get()(data, this);
}

}  // namespace web
