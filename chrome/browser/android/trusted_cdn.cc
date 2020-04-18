// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/trusted_cdn.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "url/gurl.h"

namespace trusted_cdn {

constexpr char kDefaultTrustedCDNBaseURL[] = "https://cdn.ampproject.org";

bool IsTrustedCDN(const GURL& url) {
  if (!base::FeatureList::IsEnabled(features::kShowTrustedPublisherURL))
    return false;

  // Use a static local (without destructor) to construct the base URL only
  // once. |trusted_cdn_base_url| is initialized with the result of an
  // immediately evaluated lambda, which allows wrapping the code in a single
  // expression.
  static const base::NoDestructor<GURL> trusted_cdn_base_url([]() {
    const base::CommandLine* command_line =
        base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(switches::kTrustedCDNBaseURLForTests)) {
      GURL base_url(command_line->GetSwitchValueASCII(
          switches::kTrustedCDNBaseURLForTests));
      LOG_IF(WARNING, !base_url.is_valid()) << "Invalid trusted CDN base URL: "
                                            << base_url.possibly_invalid_spec();
      return base_url;
    }

    return GURL(kDefaultTrustedCDNBaseURL);
  }());

  // Allow any subdomain of the base URL.
  return url.DomainIs(trusted_cdn_base_url->host_piece()) &&
         (url.scheme_piece() == trusted_cdn_base_url->scheme_piece()) &&
         (url.EffectiveIntPort() == trusted_cdn_base_url->EffectiveIntPort());
}

}  // namespace trusted_cdn
