// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/client_hints/client_hints.h"

#include "base/macros.h"

namespace blink {

const char* const kClientHintsHeaderMapping[] = {
    "device-memory", "dpr",      "width", "viewport-width",
    "rtt",           "downlink", "ect"};

const size_t kClientHintsHeaderMappingCount =
    arraysize(kClientHintsHeaderMapping);

const char* const kWebEffectiveConnectionTypeMapping[] = {
    "4g" /* Unknown */, "4g" /* Offline */, "slow-2g" /* Slow 2G */,
    "2g" /* 2G */,      "3g" /* 3G */,      "4g" /* 4G */
};

const size_t kWebEffectiveConnectionTypeMappingCount =
    arraysize(kWebEffectiveConnectionTypeMapping);

}  // namespace blink
