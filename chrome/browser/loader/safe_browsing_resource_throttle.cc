// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/loader/safe_browsing_resource_throttle.h"

#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "chrome/browser/safe_browsing/url_checker_delegate_impl.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/database_manager.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"

content::ResourceThrottle* MaybeCreateSafeBrowsingResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    safe_browsing::SafeBrowsingService* sb_service,
    const ProfileIOData* io_data) {
  if (!sb_service->database_manager()->IsSupported())
    return nullptr;

  // No need to create resource throttle if request url is whitelisted by
  // enterprise policy.
  if (io_data &&
      safe_browsing::IsURLWhitelistedByPolicy(
          request->url(), io_data->safe_browsing_whitelist_domains())) {
    return nullptr;
  }

  return new SafeBrowsingParallelResourceThrottle(request, resource_type,
                                                  sb_service);
}

SafeBrowsingParallelResourceThrottle::SafeBrowsingParallelResourceThrottle(
    const net::URLRequest* request,
    content::ResourceType resource_type,
    safe_browsing::SafeBrowsingService* sb_service)
    : safe_browsing::BaseParallelResourceThrottle(
          request,
          resource_type,
          base::MakeRefCounted<safe_browsing::UrlCheckerDelegateImpl>(
              sb_service->database_manager(),
              sb_service->ui_manager())) {}

SafeBrowsingParallelResourceThrottle::~SafeBrowsingParallelResourceThrottle() =
    default;

const char* SafeBrowsingParallelResourceThrottle::GetNameForLogging() const {
  return "SafeBrowsingParallelResourceThrottle";
}
