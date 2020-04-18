// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_UPGRADE_METRICS_PROVIDER_H_
#define CHROME_BROWSER_METRICS_UPGRADE_METRICS_PROVIDER_H_

#include "base/macros.h"
#include "components/metrics/metrics_provider.h"

// UpgradeMetricsProvider groups various constants and functions used for
// reporting extension IDs with UMA reports (after hashing the extension IDs
// for privacy).
class UpgradeMetricsProvider : public metrics::MetricsProvider {
 public:
  UpgradeMetricsProvider();
  ~UpgradeMetricsProvider() override;

  // metrics::MetricsProvider:
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UpgradeMetricsProvider);
};

#endif  // CHROME_BROWSER_METRICS_UPGRADE_METRICS_PROVIDER_H_
