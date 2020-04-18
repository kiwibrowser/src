// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/common_features.h"

namespace subresource_filter {

const base::Feature kAdTagging{"AdTagging", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kDelayUnsafeAds{"DelayUnsafeAds",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const char kInsecureDelayParam[] = "insecure_delay";
const char kNonIsolatedDelayParam[] = "non_isolated_delay";

}  // namespace subresource_filter
