// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/content_activation_list_utils.h"

#include <string>

namespace subresource_filter {

ActivationList GetListForThreatTypeAndMetadata(
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& threat_type_metadata,
    bool* warning) {
  DCHECK(warning);
  bool is_phishing_interstitial =
      (threat_type == safe_browsing::SB_THREAT_TYPE_URL_PHISHING);
  bool is_soc_engineering_ads_interstitial =
      threat_type_metadata.threat_pattern_type ==
      safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS;
  bool subresource_filter =
      (threat_type == safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER);
  if (is_phishing_interstitial) {
    if (is_soc_engineering_ads_interstitial) {
      return ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL;
    }
    return ActivationList::PHISHING_INTERSTITIAL;
  } else if (subresource_filter) {
    auto it = threat_type_metadata.subresource_filter_match.find(
        safe_browsing::SubresourceFilterType::BETTER_ADS);
    if (it != threat_type_metadata.subresource_filter_match.end()) {
      *warning = it->second == safe_browsing::SubresourceFilterLevel::WARN;
      return ActivationList::BETTER_ADS;
    }
    // Keep a generic subresource_filter list without warning implemented, for
    // subresource filter matches with no metadata.
    if (threat_type_metadata.subresource_filter_match.empty())
      return ActivationList::SUBRESOURCE_FILTER;
  }

  return ActivationList::NONE;
}

}  // namespace subresource_filter
