// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_url_request.h"
#include "net/url_request/url_request.h"

namespace content {

// static
std::unique_ptr<AppCacheURLRequest> AppCacheURLRequest::Create(
    net::URLRequest* url_request) {
  std::unique_ptr<AppCacheURLRequest> request(
      new AppCacheURLRequest(url_request));
  return request;
}

const GURL& AppCacheURLRequest::GetURL() const {
  return url_request_->url();
}

const std::string& AppCacheURLRequest::GetMethod() const {
  return url_request_->method();
}

const GURL& AppCacheURLRequest::GetSiteForCookies() const {
  return url_request_->site_for_cookies();
}

const GURL AppCacheURLRequest::GetReferrer() const {
  return GURL(url_request_->referrer());
}

bool AppCacheURLRequest::IsSuccess() const {
  return url_request_->status().is_success();
}

bool AppCacheURLRequest::IsCancelled() const {
  return url_request_->status().status() == net::URLRequestStatus::CANCELED;
}

bool AppCacheURLRequest::IsError() const {
  return !url_request_->status().is_success();
}

int AppCacheURLRequest::GetResponseCode() const {
  return url_request_->GetResponseCode();
}

std::string AppCacheURLRequest::GetResponseHeaderByName(
    const std::string& name) const {
  std::string header;
  url_request_->GetResponseHeaderByName(name, &header);
  return header;
}

net::URLRequest* AppCacheURLRequest::GetURLRequest() {
  return url_request_;
}

AppCacheURLRequest* AppCacheURLRequest::AsURLRequest() {
  return this;
}

AppCacheURLRequest::AppCacheURLRequest(net::URLRequest* url_request)
    : url_request_(url_request) {}

AppCacheURLRequest::~AppCacheURLRequest() {}

}  // namespace content
