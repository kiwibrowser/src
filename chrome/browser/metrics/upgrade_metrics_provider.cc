// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/upgrade_metrics_provider.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/upgrade_detector.h"

UpgradeMetricsProvider::UpgradeMetricsProvider() {}

UpgradeMetricsProvider::~UpgradeMetricsProvider() {}

void UpgradeMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  UpgradeDetector* upgrade_detector = UpgradeDetector::GetInstance();
  UMA_HISTOGRAM_ENUMERATION(
      "UpgradeDetector.NotificationStage",
      upgrade_detector->upgrade_notification_stage(),
      UpgradeDetector::kUpgradeNotificationAnnoyanceLevelCount);
}
