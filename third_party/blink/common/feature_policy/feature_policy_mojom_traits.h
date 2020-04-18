// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_COMMON_FEATURE_POLICY_FEATURE_POLICY_MOJOM_TRAITS_H_
#define THIRD_PARTY_BLINK_COMMON_FEATURE_POLICY_FEATURE_POLICY_MOJOM_TRAITS_H_

#include <vector>

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/blink/common/common_export.h"
#include "third_party/blink/public/common/feature_policy/feature_policy.h"
#include "third_party/blink/public/common/frame/sandbox_flags.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom-shared.h"

namespace mojo {

#define STATIC_ASSERT_ENUM(a, b)                            \
  static_assert(static_cast<int>(a) == static_cast<int>(b), \
                "mismatching enum : " #a)

// TODO(crbug.com/789818) - Merge these 2 WebSandboxFlags enums.
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kNone,
                   ::blink::mojom::WebSandboxFlags::kNone);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kNavigation,
                   ::blink::mojom::WebSandboxFlags::kNavigation);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kPlugins,
                   ::blink::mojom::WebSandboxFlags::kPlugins);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kOrigin,
                   ::blink::mojom::WebSandboxFlags::kOrigin);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kForms,
                   ::blink::mojom::WebSandboxFlags::kForms);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kScripts,
                   ::blink::mojom::WebSandboxFlags::kScripts);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kTopNavigation,
                   ::blink::mojom::WebSandboxFlags::kTopNavigation);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kPopups,
                   ::blink::mojom::WebSandboxFlags::kPopups);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kAutomaticFeatures,
                   ::blink::mojom::WebSandboxFlags::kAutomaticFeatures);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kPointerLock,
                   ::blink::mojom::WebSandboxFlags::kPointerLock);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kDocumentDomain,
                   ::blink::mojom::WebSandboxFlags::kDocumentDomain);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kOrientationLock,
                   ::blink::mojom::WebSandboxFlags::kOrientationLock);
STATIC_ASSERT_ENUM(
    ::blink::WebSandboxFlags::kPropagatesToAuxiliaryBrowsingContexts,
    ::blink::mojom::WebSandboxFlags::kPropagatesToAuxiliaryBrowsingContexts);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kModals,
                   ::blink::mojom::WebSandboxFlags::kModals);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kPresentationController,
                   ::blink::mojom::WebSandboxFlags::kPresentationController);
STATIC_ASSERT_ENUM(
    ::blink::WebSandboxFlags::kTopNavigationByUserActivation,
    ::blink::mojom::WebSandboxFlags::kTopNavigationByUserActivation);
STATIC_ASSERT_ENUM(::blink::WebSandboxFlags::kDownloads,
                   ::blink::mojom::WebSandboxFlags::kDownloads);

template <>
struct BLINK_COMMON_EXPORT
    EnumTraits<blink::mojom::WebSandboxFlags, blink::WebSandboxFlags> {
  static blink::mojom::WebSandboxFlags ToMojom(blink::WebSandboxFlags flags) {
    return static_cast<blink::mojom::WebSandboxFlags>(flags);
  }
  static bool FromMojom(blink::mojom::WebSandboxFlags in,
                        blink::WebSandboxFlags* out) {
    *out = static_cast<blink::WebSandboxFlags>(in);
    return true;
  }
};

template <>
class BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::ParsedFeaturePolicyDeclarationDataView,
                 blink::ParsedFeaturePolicyDeclaration> {
 public:
  static blink::mojom::FeaturePolicyFeature feature(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.feature;
  }
  static bool matches_all_origins(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.matches_all_origins;
  }
  static const std::vector<url::Origin>& origins(
      const blink::ParsedFeaturePolicyDeclaration& policy) {
    return policy.origins;
  }

  static bool Read(blink::mojom::ParsedFeaturePolicyDeclarationDataView in,
                   blink::ParsedFeaturePolicyDeclaration* out);
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_COMMON_FEATURE_POLICY_FEATURE_POLICY_MOJOM_TRAITS_H_
