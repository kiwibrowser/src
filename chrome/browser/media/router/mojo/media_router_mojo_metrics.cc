// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_mojo_metrics.h"

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/version.h"
#include "components/version_info/version_info.h"
#include "extensions/common/extension.h"

namespace media_router {

// static
constexpr char MediaRouterMojoMetrics::kHistogramProviderCreateRouteResult[] =
    "MediaRouter.Provider.CreateRoute.Result";
constexpr char
    MediaRouterMojoMetrics::kHistogramProviderCreateRouteResultWiredDisplay[] =
        "MediaRouter.Provider.CreateRoute.Result.WiredDisplay";
constexpr char MediaRouterMojoMetrics::kHistogramProviderJoinRouteResult[] =
    "MediaRouter.Provider.JoinRoute.Result";
constexpr char
    MediaRouterMojoMetrics::kHistogramProviderJoinRouteResultWiredDisplay[] =
        "MediaRouter.Provider.JoinRoute.Result.WiredDisplay";
constexpr char
    MediaRouterMojoMetrics::kHistogramProviderRouteControllerCreationOutcome[] =
        "MediaRouter.Provider.RouteControllerCreationOutcome";
constexpr char
    MediaRouterMojoMetrics::kHistogramProviderTerminateRouteResult[] =
        "MediaRouter.Provider.TerminateRoute.Result";
constexpr char MediaRouterMojoMetrics::
    kHistogramProviderTerminateRouteResultWiredDisplay[] =
        "MediaRouter.Provider.TerminateRoute.Result.WiredDisplay";
constexpr char MediaRouterMojoMetrics::kHistogramProviderVersion[] =
    "MediaRouter.Provider.Version";
constexpr char MediaRouterMojoMetrics::kHistogramProviderWakeReason[] =
    "MediaRouter.Provider.WakeReason";
constexpr char MediaRouterMojoMetrics::kHistogramProviderWakeup[] =
    "MediaRouter.Provider.Wakeup";

// static
void MediaRouterMojoMetrics::RecordMediaRouteProviderWakeReason(
    MediaRouteProviderWakeReason reason) {
  DCHECK_LT(static_cast<int>(reason),
            static_cast<int>(MediaRouteProviderWakeReason::TOTAL_COUNT));
  UMA_HISTOGRAM_ENUMERATION(
      kHistogramProviderWakeReason, static_cast<int>(reason),
      static_cast<int>(MediaRouteProviderWakeReason::TOTAL_COUNT));
}

// static
void MediaRouterMojoMetrics::RecordMediaRouteProviderVersion(
    const extensions::Extension& extension) {
  MediaRouteProviderVersion version = MediaRouteProviderVersion::UNKNOWN;
  version = GetMediaRouteProviderVersion(extension.version(),
                                         version_info::GetVersion());

  DCHECK_LT(static_cast<int>(version),
            static_cast<int>(MediaRouteProviderVersion::TOTAL_COUNT));
  UMA_HISTOGRAM_ENUMERATION(
      kHistogramProviderVersion, static_cast<int>(version),
      static_cast<int>(MediaRouteProviderVersion::TOTAL_COUNT));
}

// static
void MediaRouterMojoMetrics::RecordMediaRouteProviderWakeup(
    MediaRouteProviderWakeup wakeup) {
  DCHECK_LT(static_cast<int>(wakeup),
            static_cast<int>(MediaRouteProviderWakeup::TOTAL_COUNT));
  UMA_HISTOGRAM_ENUMERATION(
      kHistogramProviderWakeup, static_cast<int>(wakeup),
      static_cast<int>(MediaRouteProviderWakeup::TOTAL_COUNT));
}

// static
void MediaRouterMojoMetrics::RecordCreateRouteResultCode(
    MediaRouteProviderId provider_id,
    RouteRequestResult::ResultCode result_code) {
  DCHECK_LT(result_code, RouteRequestResult::TOTAL_COUNT);
  switch (provider_id) {
    case MediaRouteProviderId::WIRED_DISPLAY:
      UMA_HISTOGRAM_ENUMERATION(kHistogramProviderCreateRouteResultWiredDisplay,
                                result_code, RouteRequestResult::TOTAL_COUNT);
      break;
    case MediaRouteProviderId::EXTENSION:
    // TODO(crbug.com/809249): Implement Cast-specific metric.
    case MediaRouteProviderId::CAST:
    // TODO(crbug.com/808720): Implement DIAL-specific metric.
    case MediaRouteProviderId::DIAL:
    case MediaRouteProviderId::UNKNOWN:
      UMA_HISTOGRAM_ENUMERATION(kHistogramProviderCreateRouteResult,
                                result_code, RouteRequestResult::TOTAL_COUNT);
      break;
  }
}

// static
void MediaRouterMojoMetrics::RecordJoinRouteResultCode(
    MediaRouteProviderId provider_id,
    RouteRequestResult::ResultCode result_code) {
  DCHECK_LT(result_code, RouteRequestResult::ResultCode::TOTAL_COUNT);
  switch (provider_id) {
    case MediaRouteProviderId::WIRED_DISPLAY:
      UMA_HISTOGRAM_ENUMERATION(kHistogramProviderJoinRouteResultWiredDisplay,
                                result_code, RouteRequestResult::TOTAL_COUNT);
      break;
    case MediaRouteProviderId::EXTENSION:
    // TODO(crbug.com/809249): Implement Cast-specific metric.
    case MediaRouteProviderId::CAST:
    // TODO(crbug.com/808720): Implement DIAL-specific metric.
    case MediaRouteProviderId::DIAL:
    case MediaRouteProviderId::UNKNOWN:
      UMA_HISTOGRAM_ENUMERATION(kHistogramProviderJoinRouteResult, result_code,
                                RouteRequestResult::TOTAL_COUNT);
      break;
  }
}

// static
void MediaRouterMojoMetrics::RecordMediaRouteProviderTerminateRoute(
    MediaRouteProviderId provider_id,
    RouteRequestResult::ResultCode result_code) {
  DCHECK_LT(result_code, RouteRequestResult::ResultCode::TOTAL_COUNT);
  switch (provider_id) {
    case MediaRouteProviderId::WIRED_DISPLAY:
      UMA_HISTOGRAM_ENUMERATION(
          kHistogramProviderTerminateRouteResultWiredDisplay, result_code,
          RouteRequestResult::TOTAL_COUNT);
      break;
    case MediaRouteProviderId::EXTENSION:
    // TODO(crbug.com/809249): Implement Cast-specific metric.
    case MediaRouteProviderId::CAST:
    // TODO(crbug.com/808720): Implement DIAL-specific metric.
    case MediaRouteProviderId::DIAL:
    case MediaRouteProviderId::UNKNOWN:
      UMA_HISTOGRAM_ENUMERATION(kHistogramProviderTerminateRouteResult,
                                result_code, RouteRequestResult::TOTAL_COUNT);
      break;
  }
}

// static
void MediaRouterMojoMetrics::RecordMediaRouteControllerCreationResult(
    bool success) {
  UMA_HISTOGRAM_BOOLEAN(kHistogramProviderRouteControllerCreationOutcome,
                        success);
}

// static
MediaRouteProviderVersion MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
    const base::Version& extension_version,
    const base::Version& browser_version) {
  if (!extension_version.IsValid() || extension_version.components().empty() ||
      !browser_version.IsValid() || browser_version.components().empty()) {
    return MediaRouteProviderVersion::UNKNOWN;
  }

  uint32_t extension_major = extension_version.components()[0];
  uint32_t browser_major = browser_version.components()[0];
  // Sanity check.
  if (extension_major == 0 || browser_major == 0) {
    return MediaRouteProviderVersion::UNKNOWN;
  } else if (extension_major >= browser_major) {
    return MediaRouteProviderVersion::SAME_VERSION_AS_CHROME;
  } else if (browser_major - extension_major == 1) {
    return MediaRouteProviderVersion::ONE_VERSION_BEHIND_CHROME;
  } else {
    return MediaRouteProviderVersion::MULTIPLE_VERSIONS_BEHIND_CHROME;
  }
}

}  // namespace media_router
