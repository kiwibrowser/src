// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_POLICY_FEATURE_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_POLICY_FEATURE_POLICY_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/common/feature_policy/feature_policy.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "url/origin.h"

#include <memory>

namespace blink {

// Returns a map between feature name (string) and mojom::FeaturePolicyFeature
// (enum).
typedef HashMap<String, mojom::FeaturePolicyFeature> FeatureNameMap;
PLATFORM_EXPORT const FeatureNameMap& GetDefaultFeatureNameMap();

// Converts a header policy string into a vector of whitelists, one for each
// feature specified. Unrecognized features are filtered out. If |messages|
// is not null, then any message in the input will cause a warning message to be
// appended to it.
// Example of a feature policy string:
//     "vibrate a.com b.com; fullscreen 'none'; payment 'self', payment *".
PLATFORM_EXPORT ParsedFeaturePolicy
ParseFeaturePolicyHeader(const String& policy,
                         scoped_refptr<const SecurityOrigin>,
                         Vector<String>* messages);

// Converts a container policy string into a vector of whitelists, given self
// and src origins provided, one for each feature specified. Unrecognized
// features are filtered out. If |messages| is not null, then any message in the
// input will cause as warning message to be appended to it.
// Example of a feature policy string:
//     "vibrate a.com 'src'; fullscreen 'none'; payment 'self', payment *".
PLATFORM_EXPORT ParsedFeaturePolicy
ParseFeaturePolicyAttribute(const String& policy,
                            scoped_refptr<const SecurityOrigin> self_origin,
                            scoped_refptr<const SecurityOrigin> src_origin,
                            Vector<String>* messages);

// Converts a feature policy string into a vector of whitelists (see comments
// above), with an explicit FeatureNameMap. This algorithm is called by both
// header policy parsing and container policy parsing. |self_origin| and
// |src_origin| are both nullable.
PLATFORM_EXPORT ParsedFeaturePolicy
ParseFeaturePolicy(const String& policy,
                   scoped_refptr<const SecurityOrigin> self_origin,
                   scoped_refptr<const SecurityOrigin> src_origin,
                   Vector<String>* messages,
                   const FeatureNameMap& feature_names);

// Verifies whether feature policy is enabled and |feature| is supported in
// feature policy.
PLATFORM_EXPORT bool IsSupportedInFeaturePolicy(mojom::FeaturePolicyFeature);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_POLICY_FEATURE_POLICY_H_
