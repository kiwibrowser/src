// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_restrictions/browser/web_restrictions_resource_throttle.h"

#include "base/bind.h"
#include "components/web_restrictions/browser/web_restrictions_client.h"
#include "net/base/net_errors.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"

namespace web_restrictions {

WebRestrictionsResourceThrottle::WebRestrictionsResourceThrottle(
    WebRestrictionsClient* provider,
    const GURL& request_url,
    bool is_main_frame)
    : provider_(provider),
      request_url_(request_url),
      is_main_frame_(is_main_frame),
      weak_ptr_factory_(this) {}

WebRestrictionsResourceThrottle::~WebRestrictionsResourceThrottle() {}

void WebRestrictionsResourceThrottle::WillStartRequest(bool* defer) {
  *defer = ShouldDefer(request_url_);
}

void WebRestrictionsResourceThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  *defer = ShouldDefer(redirect_info.new_url);
}

const char* WebRestrictionsResourceThrottle::GetNameForLogging() const {
  return "WebRestrictionsResourceThrottle";
}

bool WebRestrictionsResourceThrottle::ShouldDefer(const GURL& url) {
  // For requests to function correctly, we need to allow subresources.
  if (provider_->SupportsRequest() && !is_main_frame_)
    return false;
  UrlAccess access = provider_->ShouldProceed(
      is_main_frame_, url.spec(),
      base::Bind(&WebRestrictionsResourceThrottle::OnCheckResult,
                 weak_ptr_factory_.GetWeakPtr()));
  if (access == DISALLOW)
    OnCheckResult(false);
  return access == PENDING;
}

void WebRestrictionsResourceThrottle::OnCheckResult(const bool should_proceed) {
  if (should_proceed) {
    Resume();
  } else {
    CancelWithError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
  }
}

}  // namespace web_restrictions
