// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/interstitials/chrome_metrics_helper.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "components/history/core/browser/history_service.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
#include "chrome/browser/ssl/captive_portal_metrics_recorder.h"
#endif

ChromeMetricsHelper::ChromeMetricsHelper(
    content::WebContents* web_contents,
    const GURL& request_url,
    const security_interstitials::MetricsHelper::ReportDetails settings)
    : security_interstitials::MetricsHelper(
          request_url,
          settings,
          HistoryServiceFactory::GetForProfile(
              Profile::FromBrowserContext(web_contents->GetBrowserContext()),
              ServiceAccessType::EXPLICIT_ACCESS)),
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
      web_contents_(web_contents),
#endif
      request_url_(request_url) {
}

ChromeMetricsHelper::~ChromeMetricsHelper() {}

void ChromeMetricsHelper::StartRecordingCaptivePortalMetrics(bool overridable) {
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
  captive_portal_recorder_.reset(
      new CaptivePortalMetricsRecorder(web_contents_, overridable));
#endif
}

void ChromeMetricsHelper::RecordExtraShutdownMetrics() {
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
  // The captive portal metrics should be recorded when the interstitial is
  // closing (or destructing).
  if (captive_portal_recorder_)
    captive_portal_recorder_->RecordCaptivePortalUMAStatistics();
#endif
}

