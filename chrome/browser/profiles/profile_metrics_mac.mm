// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_metrics.h"

#include <stddef.h>
#include <stdint.h>

#include "base/numerics/safe_conversions.h"
#include "chrome/browser/mac/keystone_glue.h"

void ProfileMetrics::UpdateReportedOSProfileStatistics(
    size_t active, size_t signedin) {
  if (base::IsValueInRangeForNumericType<uint32_t>(active) &&
      base::IsValueInRangeForNumericType<uint32_t>(signedin)) {
    [[KeystoneGlue defaultKeystoneGlue]
        updateProfileCountsWithNumProfiles:active
                       numSignedInProfiles:signedin];
  }
}
