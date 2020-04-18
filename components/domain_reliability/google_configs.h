// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOMAIN_RELIABILITY_GOOGLE_CONFIGS_H_
#define COMPONENTS_DOMAIN_RELIABILITY_GOOGLE_CONFIGS_H_

#include <memory>

#include "components/domain_reliability/config.h"
#include "components/domain_reliability/domain_reliability_export.h"

namespace domain_reliability {

void DOMAIN_RELIABILITY_EXPORT GetAllGoogleConfigs(
    std::vector<std::unique_ptr<DomainReliabilityConfig>>* configs_out);

}  // namespace domain_reliability

#endif // COMPONENTS_DOMAIN_RELIABILITY_GOOGLE_CONFIGS_H_
