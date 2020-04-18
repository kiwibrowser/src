// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/intercept_download_resource_throttle.h"

#include "base/strings/string_util.h"
#include "chrome/browser/android/download/download_controller_base.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"

InterceptDownloadResourceThrottle::InterceptDownloadResourceThrottle(
    net::URLRequest* request,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter)
    : request_(request),
      wc_getter_(wc_getter),
      weak_factory_(this) {
}

InterceptDownloadResourceThrottle::~InterceptDownloadResourceThrottle() =
    default;

void InterceptDownloadResourceThrottle::WillProcessResponse(bool* defer) {
  if (request_->url_chain().empty())
    return;

  GURL url = request_->url_chain().back();
  if (!url.SchemeIsHTTPOrHTTPS())
    return;

  if (request_->method() != net::HttpRequestHeaders::kGetMethod)
    return;

  net::HttpRequestHeaders headers;
  if (!request_->GetFullRequestHeaders(&headers))
    return;

  std::string mime_type;
  request_->response_headers()->GetMimeType(&mime_type);
  if (!base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmMessageMimeType) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmContentMimeType) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmRightsMimeType1) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmRightsMimeType2)) {
    return;
  }

  net::CookieStore* cookie_store = request_->context()->cookie_store();
  if (cookie_store) {
    // Cookie is obtained via asynchonous call. Setting |*defer| to true
    // keeps the throttle alive in the meantime.
    *defer = true;
    net::CookieOptions options;
    options.set_include_httponly();
    cookie_store->GetCookieListWithOptionsAsync(
        request_->url(),
        options,
        base::Bind(&InterceptDownloadResourceThrottle::CheckCookiePolicy,
                   weak_factory_.GetWeakPtr()));
  } else {
    // Can't get any cookies, start android download.
    StartDownload(DownloadInfo(request_));
  }
}

const char* InterceptDownloadResourceThrottle::GetNameForLogging() const {
  return "InterceptDownloadResourceThrottle";
}

void InterceptDownloadResourceThrottle::CheckCookiePolicy(
    const net::CookieList& cookie_list) {
  DownloadInfo info(request_);
  if (request_->context()->network_delegate()->CanGetCookies(*request_,
                                                             cookie_list)) {
    std::string cookie = net::CanonicalCookie::BuildCookieLine(cookie_list);
    if (!cookie.empty())
      info.cookie = cookie;
  }
  StartDownload(info);
}

void InterceptDownloadResourceThrottle::StartDownload(
    const DownloadInfo& info) {
  DownloadControllerBase::Get()->CreateAndroidDownload(wc_getter_, info);
  Cancel();
}
