// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/app_categorizer.h"

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace {
// Note: all domain names here must be in lowercase (see GURL::DomainIs, which
// properly handles sub-domains).

const char* const kPredefinedHangoutsDomains[] = {
  "hangouts.google.com",
  "meet.google.com",
  "talkgadget.google.com",
  "plus.google.com",
  "plus.sandbox.google.com"
};

bool IsInWhitelistedDomain(
    const GURL& url, const char* const domains[], size_t number_of_domains) {
  for (size_t i = 0; i < number_of_domains; ++i) {
    if (url.DomainIs(domains[i])) {
      return true;
    }
  }

  return false;
}

}  // namespace

bool AppCategorizer::IsHangoutsUrl(const GURL& url) {
  // Whitelisted apps must be served over https.
  return url.SchemeIsCryptographic() &&
      base::StartsWith(url.path(), "/hangouts/",
                       base::CompareCase::INSENSITIVE_ASCII) &&
      IsInWhitelistedDomain(
          url,
          kPredefinedHangoutsDomains,
          arraysize(kPredefinedHangoutsDomains));
}

bool AppCategorizer::IsWhitelistedApp(
    const GURL& manifest_url, const GURL& app_url) {
  // Whitelisted apps must be served over https.
  if (!app_url.SchemeIsCryptographic())
    return false;

  bool is_hangouts_app =
      manifest_url.SchemeIsFileSystem() &&
      manifest_url.inner_url() != NULL &&
      manifest_url.inner_url()->SchemeIsCryptographic() &&
      // The manifest must be loaded from the host's FileSystem.
      (manifest_url.inner_url()->host() == app_url.host()) &&
      IsHangoutsUrl(app_url);

  return is_hangouts_app;
}
