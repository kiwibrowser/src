// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/page_load_capping/chrome_page_load_capping_features.h"

namespace data_use_measurement {
namespace page_load_capping {
namespace features {

// Enables tracking heavy pages in Chrome.
const base::Feature kDetectingHeavyPages{"DetectingHeavyPages",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace page_load_capping
}  // namespace data_use_measurement
