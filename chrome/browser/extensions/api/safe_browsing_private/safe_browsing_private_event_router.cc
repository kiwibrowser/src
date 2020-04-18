// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router.h"

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/safe_browsing_private.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/event_router.h"
#include "url/gurl.h"

namespace extensions {

SafeBrowsingPrivateEventRouter::SafeBrowsingPrivateEventRouter(
    content::BrowserContext* context)
    : context_(context), event_router_(nullptr) {
  event_router_ = EventRouter::Get(context_);
}

SafeBrowsingPrivateEventRouter::~SafeBrowsingPrivateEventRouter() {}

void SafeBrowsingPrivateEventRouter::OnPolicySpecifiedPasswordReuseDetected(
    const GURL& url,
    const std::string& user_name,
    bool is_phishing_url) {
  // |event_router_| can be null in tests.
  if (!event_router_)
    return;

  api::safe_browsing_private::PolicySpecifiedPasswordReuse params;
  params.url = url.spec();
  params.user_name = user_name;
  params.is_phishing_url = is_phishing_url;

  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(params.ToValue());

  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_POLICY_SPECIFIED_PASSWORD_REUSE_DETECTED,
      api::safe_browsing_private::OnPolicySpecifiedPasswordReuseDetected::
          kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

void SafeBrowsingPrivateEventRouter::OnPolicySpecifiedPasswordChanged(
    const std::string& user_name) {
  // |event_router_| can be null in tests.
  if (!event_router_)
    return;

  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(std::make_unique<base::Value>(user_name));
  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_POLICY_SPECIFIED_PASSWORD_CHANGED,
      api::safe_browsing_private::OnPolicySpecifiedPasswordChanged::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

void SafeBrowsingPrivateEventRouter::OnDangerousDownloadOpened(
    const GURL& url,
    const std::string& file_name,
    const std::string& download_digest_sha256,
    const std::string& user_name) {
  // |event_router_| can be null in tests.
  if (!event_router_)
    return;

  api::safe_browsing_private::DangerousDownloadInfo params;
  params.url = url.spec();
  params.file_name = file_name;
  params.download_digest_sha256 = download_digest_sha256;
  params.user_name = user_name;
  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(params.ToValue());

  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_DANGEROUS_DOWNLOAD_OPENED,
      api::safe_browsing_private::OnDangerousDownloadOpened::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

void SafeBrowsingPrivateEventRouter::OnSecurityInterstitialShown(
    const GURL& url,
    const std::string& reason,
    int net_error_code,
    const std::string& user_name) {
  // |event_router_| can be null in tests.
  if (!event_router_)
    return;

  api::safe_browsing_private::InterstitialInfo params;
  params.url = url.spec();
  params.reason = reason;
  if (net_error_code < 0) {
    params.net_error_code =
        std::make_unique<std::string>(base::NumberToString(net_error_code));
  }
  params.user_name = user_name;

  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(params.ToValue());

  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_SECURITY_INTERSTITIAL_SHOWN,
      api::safe_browsing_private::OnSecurityInterstitialShown::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

void SafeBrowsingPrivateEventRouter::OnSecurityInterstitialProceeded(
    const GURL& url,
    const std::string& reason,
    int net_error_code,
    const std::string& user_name) {
  // |event_router_| can be null in tests.
  if (!event_router_)
    return;

  api::safe_browsing_private::InterstitialInfo params;
  params.url = url.spec();
  params.reason = reason;
  if (net_error_code < 0) {
    params.net_error_code =
        std::make_unique<std::string>(base::NumberToString(net_error_code));
  }
  params.user_name = user_name;

  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(params.ToValue());

  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_SECURITY_INTERSTITIAL_PROCEEDED,
      api::safe_browsing_private::OnSecurityInterstitialProceeded::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

}  // namespace extensions
