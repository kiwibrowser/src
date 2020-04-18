// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_METRICS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_METRICS_H_

#include "base/time/time.h"
#include "components/cast_channel/cast_message_util.h"

namespace media_router {

// Histogram names for app availability.
static constexpr char kHistogramAppAvailabilitySuccess[] =
    "MediaRouter.Cast.App.Availability.Success";
static constexpr char kHistogramAppAvailabilityFailure[] =
    "MediaRouter.Cast.App.Availability.Failure";

// Records the result of an app availability request and how long it took.
// If |result| is kUnknown, then a failure is recorded. Otherwise, a success
// is recorded.
void RecordAppAvailabilityResult(cast_channel::GetAppAvailabilityResult result,
                                 base::TimeDelta duration);

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_METRICS_H_
