// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INTERSTITIALS_CHROME_METRICS_HELPER_H_
#define CHROME_BROWSER_INTERSTITIALS_CHROME_METRICS_HELPER_H_

#include <string>

#include "base/macros.h"
#include "chrome/common/buildflags.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

class CaptivePortalMetricsRecorder;

// This class adds desktop-Chrome-specific metrics to the
// security_interstitials::MetricsHelper.
// TODO(crbug.com/812808): Refactor out the use of this class if possible.

// This class is meant to be used on the UI thread for captive portal metrics.
class ChromeMetricsHelper : public security_interstitials::MetricsHelper {
 public:
  ChromeMetricsHelper(
      content::WebContents* web_contents,
      const GURL& url,
      const security_interstitials::MetricsHelper::ReportDetails settings);
  ~ChromeMetricsHelper() override;

  void StartRecordingCaptivePortalMetrics(bool overridable);

 protected:
  // security_interstitials::MetricsHelper methods:
  void RecordExtraShutdownMetrics() override;

 private:
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
  content::WebContents* web_contents_;
#endif
  const GURL request_url_;
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
  std::unique_ptr<CaptivePortalMetricsRecorder> captive_portal_recorder_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeMetricsHelper);
};

#endif  // CHROME_BROWSER_INTERSTITIALS_CHROME_METRICS_HELPER_H_
