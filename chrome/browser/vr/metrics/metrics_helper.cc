// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics/metrics_helper.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/version.h"
#include "net/base/network_change_notifier.h"

namespace vr {

namespace {

constexpr char kStatusVr[] = "VR.Component.Assets.Status.OnEnter.AllVR";
constexpr char kStatusVrBrowsing[] =
    "VR.Component.Assets.Status.OnEnter.VRBrowsing";
constexpr char kStatusWebVr[] =
    "VR.Component.Assets.Status.OnEnter.WebVRPresentation";
constexpr char kLatencyVrBrowsing[] =
    "VR.Component.Assets.DurationUntilReady.OnEnter.VRBrowsing";
constexpr char kLatencyWebVr[] =
    "VR.Component.Assets.DurationUntilReady.OnEnter.WebVRPresentation";
constexpr char kLatencyRegisterComponent[] =
    "VR.Component.Assets.DurationUntilReady.OnRegisterComponent";
// TODO(tiborg): Rename VRAssetsComponentStatus and VRAssetsLoadStatus in
// enums.xml and consider merging them.
constexpr char kComponentUpdateStatus[] =
    "VR.Component.Assets.VersionAndStatus.OnUpdate";
constexpr char kAssetsLoadStatus[] =
    "VR.Component.Assets.VersionAndStatus.OnLoad";
constexpr char kNetworkConnectionTypeRegisterComponent[] =
    "VR.NetworkConnectionType.OnRegisterComponent";
constexpr char kNetworkConnectionTypeVr[] =
    "VR.NetworkConnectionType.OnEnter.AllVR";
constexpr char kNetworkConnectionTypeVrBrowsing[] =
    "VR.NetworkConnectionType.OnEnter.VRBrowsing";
constexpr char kNetworkConnectionTypeWebVr[] =
    "VR.NetworkConnectionType.OnEnter.WebVRPresentation";

const auto kMinLatency = base::TimeDelta::FromMilliseconds(500);
const auto kMaxLatency = base::TimeDelta::FromHours(1);
constexpr size_t kLatencyBucketCount = 100;

// Ensure that this stays in sync with VRComponentStatus in enums.xml.
enum class ComponentStatus : int {
  kReady = 0,
  kUnreadyOther = 1,

  // This must be last.
  kCount,
};

void LogStatus(Mode mode, ComponentStatus status) {
  switch (mode) {
    case Mode::kVr:
      UMA_HISTOGRAM_ENUMERATION(kStatusVr, status, ComponentStatus::kCount);
      return;
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_ENUMERATION(kStatusVrBrowsing, status,
                                ComponentStatus::kCount);
      return;
    case Mode::kWebXrVrPresentation:
      UMA_HISTOGRAM_ENUMERATION(kStatusWebVr, status, ComponentStatus::kCount);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

void LogLatency(Mode mode, const base::TimeDelta& latency) {
  switch (mode) {
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_CUSTOM_TIMES(kLatencyVrBrowsing, latency, kMinLatency,
                                 kMaxLatency, kLatencyBucketCount);
      return;
    case Mode::kWebXrVrPresentation:
      UMA_HISTOGRAM_CUSTOM_TIMES(kLatencyWebVr, latency, kMinLatency,
                                 kMaxLatency, kLatencyBucketCount);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

void LogConnectionType(Mode mode,
                       net::NetworkChangeNotifier::ConnectionType type) {
  switch (mode) {
    case Mode::kVr:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeVr, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeVrBrowsing, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    case Mode::kWebXrVrPresentation:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeWebVr, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

uint32_t EncodeVersionStatus(const base::Optional<base::Version>& version,
                             int status) {
  if (!version) {
    // Component version 0.0 is invalid. Thus, use it for when version is not
    // available.
    return status;
  }
  return version->components()[0] * 1000 * 1000 +
         version->components()[1] * 1000 + status;
}

}  // namespace

MetricsHelper::MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

MetricsHelper::~MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MetricsHelper::OnComponentReady(const base::Version& version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  component_ready_ = true;
  auto now = base::TimeTicks::Now();
  LogLatencyIfWaited(Mode::kVrBrowsing, now);
  LogLatencyIfWaited(Mode::kWebXrVrPresentation, now);
  OnComponentUpdated(AssetsComponentUpdateStatus::kSuccess, version);

  if (!logged_ready_duration_on_component_register_) {
    DCHECK(component_register_time_);
    auto ready_duration = now - *component_register_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES(kLatencyRegisterComponent, ready_duration,
                               kMinLatency, kMaxLatency, kLatencyBucketCount);
    logged_ready_duration_on_component_register_ = true;
  }
}

void MetricsHelper::OnEnter(Mode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LogConnectionType(mode, net::NetworkChangeNotifier::GetConnectionType());
  auto& enter_time = GetEnterTime(mode);
  if (enter_time) {
    // While we are stopping the time between entering and component readiness
    // we pretend the user is waiting on the block screen. Do not report further
    // UMA metrics.
    return;
  }
  LogStatus(mode, component_ready_ ? ComponentStatus::kReady
                                   : ComponentStatus::kUnreadyOther);
  if (!component_ready_) {
    enter_time = base::TimeTicks::Now();
  }
}

void MetricsHelper::OnRegisteredComponent() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_ENUMERATION(
      kNetworkConnectionTypeRegisterComponent,
      net::NetworkChangeNotifier::GetConnectionType(),
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
  DCHECK(!component_register_time_);
  component_register_time_ = base::TimeTicks::Now();
}

void MetricsHelper::OnComponentUpdated(
    AssetsComponentUpdateStatus status,
    const base::Optional<base::Version>& version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::UmaHistogramSparse(
      kComponentUpdateStatus,
      EncodeVersionStatus(version, static_cast<int>(status)));
}

void MetricsHelper::OnAssetsLoaded(AssetsLoadStatus status,
                                   const base::Version& component_version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::UmaHistogramSparse(
      kAssetsLoadStatus,
      EncodeVersionStatus(component_version, static_cast<int>(status)));
}

base::Optional<base::TimeTicks>& MetricsHelper::GetEnterTime(Mode mode) {
  switch (mode) {
    case Mode::kVr:
      return enter_vr_time_;
    case Mode::kVrBrowsing:
      return enter_vr_browsing_time_;
    case Mode::kWebXrVrPresentation:
      return enter_web_vr_time_;
    default:
      NOTIMPLEMENTED();
      return enter_vr_time_;
  }
}

void MetricsHelper::LogLatencyIfWaited(Mode mode, const base::TimeTicks& now) {
  auto& enter_time = GetEnterTime(mode);
  if (enter_time) {
    LogLatency(mode, now - *enter_time);
    enter_time = base::nullopt;
  }
}

}  // namespace vr
