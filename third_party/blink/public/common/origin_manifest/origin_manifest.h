// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_

#include <string>
#include <vector>

#include "third_party/blink/common/common_export.h"

namespace blink {

class BLINK_COMMON_EXPORT OriginManifest {
 public:
  // TODO(mkwst): We should be reusing existing types here, like
  // `blink::mojom::ContentSecurityPolicyType`.
  enum class ContentSecurityPolicyType {
    kReport,
    kEnforce,
  };

  enum class ActivationType {
    kFallback,
    kBaseline,
  };

  enum class FallbackDisposition {
    kBaselineOnly,
    kIncludeFallbacks,
  };

  struct ContentSecurityPolicy {
    std::string policy;
    ContentSecurityPolicyType disposition;
  };

  void AddContentSecurityPolicy(const std::string& policy,
                                ContentSecurityPolicyType disposition,
                                ActivationType is_fallback);

  const std::vector<ContentSecurityPolicy> GetContentSecurityPolicies(
      FallbackDisposition disposition) const;

 private:
  std::vector<ContentSecurityPolicy> csp_baseline_;
  std::vector<ContentSecurityPolicy> csp_fallback_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
