// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/common/feature_policy/feature_policy_mojom_traits.h"

#include "url/mojom/origin_mojom_traits.h"

namespace mojo {

bool StructTraits<blink::mojom::ParsedFeaturePolicyDeclarationDataView,
                  blink::ParsedFeaturePolicyDeclaration>::
    Read(blink::mojom::ParsedFeaturePolicyDeclarationDataView in,
         blink::ParsedFeaturePolicyDeclaration* out) {
  out->matches_all_origins = in.matches_all_origins();

  return in.ReadOrigins(&out->origins) && in.ReadFeature(&out->feature);
}

}  // namespace mojo
