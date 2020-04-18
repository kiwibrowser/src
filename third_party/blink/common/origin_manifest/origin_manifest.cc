// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/origin_manifest/origin_manifest.h"

namespace blink {

void OriginManifest::AddContentSecurityPolicy(
    const std::string& policy,
    OriginManifest::ContentSecurityPolicyType disposition,
    ActivationType activation_type) {
  OriginManifest::ContentSecurityPolicy csp = {policy, disposition};
  if (activation_type == ActivationType::kFallback) {
    csp_fallback_.push_back(csp);
  } else {
    csp_baseline_.push_back(csp);
  }
}

const std::vector<OriginManifest::ContentSecurityPolicy>
OriginManifest::GetContentSecurityPolicies(
    FallbackDisposition disposition) const {
  std::vector<OriginManifest::ContentSecurityPolicy> result;
  for (const auto& csp : csp_baseline_) {
    result.push_back(csp);
  }
  if (disposition == FallbackDisposition::kIncludeFallbacks) {
    for (const auto& csp : csp_fallback_) {
      result.push_back(csp);
    }
  }
  return result;
}

}  // namespace blink
