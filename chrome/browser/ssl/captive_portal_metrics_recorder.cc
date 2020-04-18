// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/captive_portal_metrics_recorder.h"

#include <vector>

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/captive_portal/captive_portal_service.h"
#include "chrome/browser/captive_portal/captive_portal_service_factory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"

namespace {

// Events for UMA. Do not reorder or change!
enum SSLInterstitialCauseCaptivePortal {
  CAPTIVE_PORTAL_ALL,
  CAPTIVE_PORTAL_DETECTION_ENABLED,
  CAPTIVE_PORTAL_DETECTION_ENABLED_OVERRIDABLE,
  CAPTIVE_PORTAL_PROBE_COMPLETED,
  CAPTIVE_PORTAL_PROBE_COMPLETED_OVERRIDABLE,
  CAPTIVE_PORTAL_NO_RESPONSE,
  CAPTIVE_PORTAL_NO_RESPONSE_OVERRIDABLE,
  CAPTIVE_PORTAL_DETECTED,
  CAPTIVE_PORTAL_DETECTED_OVERRIDABLE,
  UNUSED_CAPTIVE_PORTAL_EVENT,
};

void RecordCaptivePortalEventStats(SSLInterstitialCauseCaptivePortal event) {
  UMA_HISTOGRAM_ENUMERATION("interstitial.ssl.captive_portal", event,
                            UNUSED_CAPTIVE_PORTAL_EVENT);
}

}  // namespace

CaptivePortalMetricsRecorder::CaptivePortalMetricsRecorder(
    content::WebContents* web_contents,
    bool overridable)
    : overridable_(overridable),
      captive_portal_detection_enabled_(false),
      captive_portal_probe_completed_(false),
      captive_portal_no_response_(false),
      captive_portal_detected_(false) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  captive_portal_detection_enabled_ =
      CaptivePortalServiceFactory::GetForProfile(profile)->enabled();
  registrar_.Add(this, chrome::NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT,
                 content::Source<Profile>(profile));
}

CaptivePortalMetricsRecorder::~CaptivePortalMetricsRecorder() {}

void CaptivePortalMetricsRecorder::RecordCaptivePortalUMAStatistics() const {
  RecordCaptivePortalEventStats(CAPTIVE_PORTAL_ALL);
  if (captive_portal_detection_enabled_)
    RecordCaptivePortalEventStats(
        overridable_ ? CAPTIVE_PORTAL_DETECTION_ENABLED_OVERRIDABLE
                     : CAPTIVE_PORTAL_DETECTION_ENABLED);
  if (captive_portal_probe_completed_)
    RecordCaptivePortalEventStats(
        overridable_ ? CAPTIVE_PORTAL_PROBE_COMPLETED_OVERRIDABLE
                     : CAPTIVE_PORTAL_PROBE_COMPLETED);
  // Log only one of portal detected and no response results.
  if (captive_portal_detected_)
    RecordCaptivePortalEventStats(overridable_
                                      ? CAPTIVE_PORTAL_DETECTED_OVERRIDABLE
                                      : CAPTIVE_PORTAL_DETECTED);
  else if (captive_portal_no_response_)
    RecordCaptivePortalEventStats(overridable_
                                      ? CAPTIVE_PORTAL_NO_RESPONSE_OVERRIDABLE
                                      : CAPTIVE_PORTAL_NO_RESPONSE);
}

void CaptivePortalMetricsRecorder::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT, type);

  // When detection is disabled, captive portal service always sends
  // RESULT_INTERNET_CONNECTED. Ignore any probe results in that case.
  if (!captive_portal_detection_enabled_)
    return;

  captive_portal_probe_completed_ = true;
  CaptivePortalService::Results* results =
      content::Details<CaptivePortalService::Results>(details).ptr();
  // If a captive portal was detected at any point when the interstitial was
  // displayed, assume that the interstitial was caused by a captive portal.
  // Example scenario:
  // 1- Interstitial displayed and captive portal detected, setting the flag.
  // 2- Captive portal detection automatically opens portal login page.
  // 3- User logs in on the portal login page.
  // A notification will be received here for RESULT_INTERNET_CONNECTED. Make
  // sure we don't clear the captive protal flag, since the interstitial was
  // potentially caused by the captive portal.
  captive_portal_detected_ =
      captive_portal_detected_ ||
      (results->result == captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL);
  // Also keep track of non-HTTP portals and error cases.
  captive_portal_no_response_ =
      captive_portal_no_response_ ||
      (results->result == captive_portal::RESULT_NO_RESPONSE);
}
