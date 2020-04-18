// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_url_loader_request.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/redirect_util.h"
#include "net/url_request/url_request.h"

namespace content {

// static
std::unique_ptr<AppCacheURLLoaderRequest> AppCacheURLLoaderRequest::Create(
    const network::ResourceRequest& request) {
  return std::unique_ptr<AppCacheURLLoaderRequest>(
      new AppCacheURLLoaderRequest(request));
}

AppCacheURLLoaderRequest::~AppCacheURLLoaderRequest() {}

const GURL& AppCacheURLLoaderRequest::GetURL() const {
  return request_.url;
}

const std::string& AppCacheURLLoaderRequest::GetMethod() const {
  return request_.method;
}

const GURL& AppCacheURLLoaderRequest::GetSiteForCookies() const {
  return request_.site_for_cookies;
}

const GURL AppCacheURLLoaderRequest::GetReferrer() const {
  return request_.referrer;
}

bool AppCacheURLLoaderRequest::IsSuccess() const {
  if (response_.headers)
    return true;
  return false;
}

bool AppCacheURLLoaderRequest::IsCancelled() const {
  return false;
}

bool AppCacheURLLoaderRequest::IsError() const {
  return false;
}

int AppCacheURLLoaderRequest::GetResponseCode() const {
  if (response_.headers)
    return response_.headers->response_code();
  return 0;
}

std::string AppCacheURLLoaderRequest::GetResponseHeaderByName(
    const std::string& name) const {
  std::string header;
  if (response_.headers)
    response_.headers->GetNormalizedHeader(name, &header);
  return header;
}

network::ResourceRequest* AppCacheURLLoaderRequest::GetResourceRequest() {
  return &request_;
}

AppCacheURLLoaderRequest* AppCacheURLLoaderRequest::AsURLLoaderRequest() {
  return this;
}

base::WeakPtr<AppCacheURLLoaderRequest> AppCacheURLLoaderRequest::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void AppCacheURLLoaderRequest::UpdateWithRedirectInfo(
    const net::RedirectInfo& redirect_info) {
  bool not_used_clear_body;
  net::RedirectUtil::UpdateHttpRequest(request_.url, request_.method,
                                       redirect_info, &request_.headers,
                                       &not_used_clear_body);
  request_.url = redirect_info.new_url;
  request_.method = redirect_info.new_method;
  request_.referrer = GURL(redirect_info.new_referrer);
  request_.site_for_cookies = redirect_info.new_site_for_cookies;
}

AppCacheURLLoaderRequest::AppCacheURLLoaderRequest(
    const network::ResourceRequest& request)
    : request_(request), weak_factory_(this) {}

}  // namespace content
