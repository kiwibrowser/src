// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_REFERRER_POLICY_ENUM_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_REFERRER_POLICY_ENUM_TRAITS_H_

#include "base/logging.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/blink/public/platform/referrer.mojom-shared.h"
#include "third_party/blink/public/platform/web_referrer_policy.h"

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::ReferrerPolicy, ::blink::WebReferrerPolicy> {
  static ::blink::mojom::ReferrerPolicy ToMojom(
      ::blink::WebReferrerPolicy policy) {
    switch (policy) {
      case ::blink::kWebReferrerPolicyAlways:
        return ::blink::mojom::ReferrerPolicy::ALWAYS;
      case ::blink::kWebReferrerPolicyDefault:
        return ::blink::mojom::ReferrerPolicy::DEFAULT;
      case ::blink::kWebReferrerPolicyNoReferrerWhenDowngrade:
        return ::blink::mojom::ReferrerPolicy::NO_REFERRER_WHEN_DOWNGRADE;
      case ::blink::kWebReferrerPolicyNever:
        return ::blink::mojom::ReferrerPolicy::NEVER;
      case ::blink::kWebReferrerPolicyOrigin:
        return ::blink::mojom::ReferrerPolicy::ORIGIN;
      case ::blink::kWebReferrerPolicyOriginWhenCrossOrigin:
        return ::blink::mojom::ReferrerPolicy::ORIGIN_WHEN_CROSS_ORIGIN;
      case ::blink::kWebReferrerPolicySameOrigin:
        return ::blink::mojom::ReferrerPolicy::SAME_ORIGIN;
      case ::blink::kWebReferrerPolicyStrictOrigin:
        return ::blink::mojom::ReferrerPolicy::STRICT_ORIGIN;
      case ::blink::
          kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin:
        return ::blink::mojom::ReferrerPolicy::
            NO_REFERRER_WHEN_DOWNGRADE_ORIGIN_WHEN_CROSS_ORIGIN;
      default:
        NOTREACHED();
        return ::blink::mojom::ReferrerPolicy::DEFAULT;
    }
  }

  static bool FromMojom(::blink::mojom::ReferrerPolicy policy,
                        ::blink::WebReferrerPolicy* out) {
    switch (policy) {
      case ::blink::mojom::ReferrerPolicy::ALWAYS:
        *out = ::blink::kWebReferrerPolicyAlways;
        return true;
      case ::blink::mojom::ReferrerPolicy::DEFAULT:
        *out = ::blink::kWebReferrerPolicyDefault;
        return true;
      case ::blink::mojom::ReferrerPolicy::NO_REFERRER_WHEN_DOWNGRADE:
        *out = ::blink::kWebReferrerPolicyNoReferrerWhenDowngrade;
        return true;
      case ::blink::mojom::ReferrerPolicy::NEVER:
        *out = ::blink::kWebReferrerPolicyNever;
        return true;
      case ::blink::mojom::ReferrerPolicy::ORIGIN:
        *out = ::blink::kWebReferrerPolicyOrigin;
        return true;
      case ::blink::mojom::ReferrerPolicy::ORIGIN_WHEN_CROSS_ORIGIN:
        *out = ::blink::kWebReferrerPolicyOriginWhenCrossOrigin;
        return true;
      case ::blink::mojom::ReferrerPolicy::SAME_ORIGIN:
        *out = ::blink::kWebReferrerPolicySameOrigin;
        return true;
      case ::blink::mojom::ReferrerPolicy::STRICT_ORIGIN:
        *out = ::blink::kWebReferrerPolicyStrictOrigin;
        return true;
      case ::blink::mojom::ReferrerPolicy::
          NO_REFERRER_WHEN_DOWNGRADE_ORIGIN_WHEN_CROSS_ORIGIN:
        *out = ::blink::
            kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin;
        return true;
      default:
        NOTREACHED();
        return false;
    }
  }
};

}  // namespace mojo

#endif
