// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_CONTENT_SECURITY_POLICY_STRUCT_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_CONTENT_SECURITY_POLICY_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/blink/public/platform/content_security_policy.mojom-shared.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::ContentSecurityPolicyType,
                  ::blink::WebContentSecurityPolicyType> {
  static ::blink::mojom::ContentSecurityPolicyType ToMojom(
      ::blink::WebContentSecurityPolicyType input) {
    switch (input) {
      case ::blink::kWebContentSecurityPolicyTypeReport:
        return ::blink::mojom::ContentSecurityPolicyType::kReport;
      case ::blink::kWebContentSecurityPolicyTypeEnforce:
        return ::blink::mojom::ContentSecurityPolicyType::kEnforce;
    }
    NOTREACHED();
    return ::blink::mojom::ContentSecurityPolicyType::kReport;
  }

  static bool FromMojom(::blink::mojom::ContentSecurityPolicyType input,
                        ::blink::WebContentSecurityPolicyType* output) {
    switch (input) {
      case ::blink::mojom::ContentSecurityPolicyType::kReport:
        *output = ::blink::kWebContentSecurityPolicyTypeReport;
        return true;
      case ::blink::mojom::ContentSecurityPolicyType::kEnforce:
        *output = ::blink::kWebContentSecurityPolicyTypeEnforce;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_CONTENT_SECURITY_POLICY_STRUCT_TRAITS_H_
