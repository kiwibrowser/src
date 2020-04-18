// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/android_metrics_provider.h"

#include "base/metrics/histogram_macros.h"
#include "base/sys_info.h"
#include "chrome/browser/android/feature_utilities.h"

namespace {

void EmitLowRamDeviceHistogram() {
  // Equivalent to UMA_HISTOGRAM_BOOLEAN with the stability flag set.
  UMA_STABILITY_HISTOGRAM_ENUMERATION(
      "MemoryAndroid.LowRamDevice", base::SysInfo::IsLowEndDevice() ? 1 : 0, 2);
}

}  // namespace

AndroidMetricsProvider::AndroidMetricsProvider() {}

AndroidMetricsProvider::~AndroidMetricsProvider() {
}

void AndroidMetricsProvider::ProvidePreviousSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  // The low-ram device status is unlikely to change between browser restarts.
  // Hence, it's safe and useful to attach this status to a previous session
  // log.
  EmitLowRamDeviceHistogram();
}

void AndroidMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  EmitLowRamDeviceHistogram();
  UMA_HISTOGRAM_ENUMERATION(
      "CustomTabs.Visible",
      chrome::android::GetCustomTabsVisibleValue(),
      chrome::android::CUSTOM_TABS_VISIBILITY_MAX);
  UMA_HISTOGRAM_BOOLEAN(
      "Android.MultiWindowMode.Active",
      chrome::android::GetIsInMultiWindowModeValue());
}
