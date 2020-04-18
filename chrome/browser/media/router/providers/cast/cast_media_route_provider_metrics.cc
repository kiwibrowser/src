// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_media_route_provider_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

namespace media_router {

void RecordAppAvailabilityResult(cast_channel::GetAppAvailabilityResult result,
                                 base::TimeDelta duration) {
  if (result == cast_channel::GetAppAvailabilityResult::kUnknown)
    UMA_HISTOGRAM_TIMES(kHistogramAppAvailabilityFailure, duration);
  else
    UMA_HISTOGRAM_TIMES(kHistogramAppAvailabilitySuccess, duration);
}

}  // namespace media_router
