// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/v4_feature_list.h"

#include "base/feature_list.h"
#include "components/safe_browsing/features.h"

namespace safe_browsing {

namespace V4FeatureList {

V4UsageStatus GetV4UsageStatus() {
#if defined(SAFE_BROWSING_DB_LOCAL)
  return V4UsageStatus::V4_ONLY;
#else
  return V4UsageStatus::V4_DISABLED;
#endif
}

}  // namespace V4FeatureList

}  // namespace safe_browsing
