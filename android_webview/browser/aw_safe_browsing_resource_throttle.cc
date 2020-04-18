// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_resource_throttle.h"

#include <memory>

#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"
#include "android_webview/browser/aw_url_checker_delegate_impl.h"
#include "components/safe_browsing/common/safebrowsing_constants.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"

namespace android_webview {
namespace {

// This is used as a user data key for net::URLRequest. Setting it indicates
// that the request is cancelled because of SafeBrowsing. It could be used, for
// example, to decide whether to override the error code with a
// SafeBrowsing-specific error code.
const void* const kCancelledBySafeBrowsingUserDataKey =
    &kCancelledBySafeBrowsingUserDataKey;

void SetCancelledBySafeBrowsing(net::URLRequest* request) {
  request->SetUserData(kCancelledBySafeBrowsingUserDataKey,
                       std::make_unique<base::SupportsUserData::Data>());
}

}  // namespace

content::ResourceThrottle* MaybeCreateAwSafeBrowsingResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
    AwSafeBrowsingWhitelistManager* whitelist_manager) {
  if (!database_manager->IsSupported())
    return nullptr;

  return new AwSafeBrowsingParallelResourceThrottle(
      request, resource_type, std::move(database_manager),
      std::move(ui_manager), whitelist_manager);
}

bool IsCancelledBySafeBrowsing(const net::URLRequest* request) {
  // Check whether the request is cancelled by
  // AwSafeBrowsingParallelResourceThrottle.
  if (request->GetUserData(kCancelledBySafeBrowsingUserDataKey))
    return true;

  // Check whether the request is cancelled by
  // safe_browsing::{Browser,Renderer}URLLoaderThrottle.
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  return request_info && request_info->GetCustomCancelReason().as_string() ==
                             safe_browsing::kCustomCancelReasonForURLLoader;
}

AwSafeBrowsingParallelResourceThrottle::AwSafeBrowsingParallelResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
    AwSafeBrowsingWhitelistManager* whitelist_manager)
    : safe_browsing::BaseParallelResourceThrottle(
          request,
          resource_type,
          new AwUrlCheckerDelegateImpl(std::move(database_manager),
                                       std::move(ui_manager),
                                       whitelist_manager)),
      request_(request) {}

AwSafeBrowsingParallelResourceThrottle::
    ~AwSafeBrowsingParallelResourceThrottle() = default;

void AwSafeBrowsingParallelResourceThrottle::CancelResourceLoad() {
  SetCancelledBySafeBrowsing(request_);
  BaseParallelResourceThrottle::CancelResourceLoad();
}

}  // namespace android_webview
