// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/utils.h"

#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_provider.h"

namespace extensions {
namespace declarative_net_request {

bool IsAPIAvailable() {
  static const bool is_api_available =
      FeatureProvider::GetAPIFeature("declarativeNetRequest")
          ->IsAvailableToEnvironment()
          .is_available();
  return is_api_available;
}

}  // namespace declarative_net_request
}  // namespace extensions
