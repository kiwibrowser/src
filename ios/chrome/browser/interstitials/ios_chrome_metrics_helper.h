// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_METRICS_HELPER_H_
#define IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_METRICS_HELPER_H_

#include <string>

#include "base/macros.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "url/gurl.h"

namespace web {
class WebState;
}

// This class provides a concrete implementation for Chrome on iOS to the
// security_interstitials::MetricsHelper class. Together, they record UMA,
// Rappor, and experience sampling metrics.
class IOSChromeMetricsHelper : public security_interstitials::MetricsHelper {
 public:
  IOSChromeMetricsHelper(
      web::WebState* web_state,
      const GURL& request_url,
      const security_interstitials::MetricsHelper::ReportDetails
          report_details);
  ~IOSChromeMetricsHelper() override;

 protected:
  // security_interstitials::MetricsHelper implementation.
  void RecordExtraUserDecisionMetrics(
      security_interstitials::MetricsHelper::Decision decision) override;
  void RecordExtraUserInteractionMetrics(
      security_interstitials::MetricsHelper::Interaction interaction) override;
  void RecordExtraShutdownMetrics() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(IOSChromeMetricsHelper);
};

#endif  // IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_METRICS_HELPER_H_
