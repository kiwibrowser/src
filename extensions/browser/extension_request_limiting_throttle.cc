// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_request_limiting_throttle.h"

#include "base/logging.h"
#include "extensions/browser/extension_throttle_entry.h"
#include "extensions/browser/extension_throttle_manager.h"
#include "net/base/net_errors.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"

namespace extensions {

ExtensionRequestLimitingThrottle::ExtensionRequestLimitingThrottle(
    const net::URLRequest* request,
    ExtensionThrottleManager* manager)
    : request_(request), manager_(manager) {
  DCHECK(manager_);
}

ExtensionRequestLimitingThrottle::~ExtensionRequestLimitingThrottle() {
}

void ExtensionRequestLimitingThrottle::WillStartRequest(bool* defer) {
  throttling_entry_ = manager_->RegisterRequestUrl(request_->url());
  if (throttling_entry_->ShouldRejectRequest(*request_))
    CancelWithError(net::ERR_TEMPORARILY_THROTTLED);
}

void ExtensionRequestLimitingThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  DCHECK_EQ(manager_->GetIdFromUrl(request_->url()),
            throttling_entry_->GetURLIdForDebugging());

  throttling_entry_->UpdateWithResponse(redirect_info.status_code);

  throttling_entry_ = manager_->RegisterRequestUrl(redirect_info.new_url);
  if (throttling_entry_->ShouldRejectRequest(*request_))
    CancelWithError(net::ERR_TEMPORARILY_THROTTLED);
}

void ExtensionRequestLimitingThrottle::WillProcessResponse(bool* defer) {
  DCHECK_EQ(manager_->GetIdFromUrl(request_->url()),
            throttling_entry_->GetURLIdForDebugging());

  if (!request_->was_cached())
    throttling_entry_->UpdateWithResponse(request_->GetResponseCode());
}

const char* ExtensionRequestLimitingThrottle::GetNameForLogging() const {
  return "ExtensionRequestLimitingThrottle";
}

}  // namespace extensions
