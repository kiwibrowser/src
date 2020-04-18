// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/captive_portal/captive_portal_login_detector.h"

#include "chrome/browser/captive_portal/captive_portal_service_factory.h"
#include "components/captive_portal/captive_portal_types.h"

using captive_portal::CaptivePortalResult;

CaptivePortalLoginDetector::CaptivePortalLoginDetector(
    Profile* profile)
    : profile_(profile),
      is_login_tab_(false),
      first_login_tab_load_(false) {
}

CaptivePortalLoginDetector::~CaptivePortalLoginDetector() {
}

void CaptivePortalLoginDetector::OnStoppedLoading() {
  // Do nothing if this is not a login tab, or if this is a login tab's first
  // load.
  if (!is_login_tab_ || first_login_tab_load_) {
    first_login_tab_load_ = false;
    return;
  }

  // The service is guaranteed to exist if |is_login_tab_| is true, since it's
  // only set to true once a captive portal is detected.
  CaptivePortalServiceFactory::GetForProfile(profile_)->DetectCaptivePortal();
}

void CaptivePortalLoginDetector::OnCaptivePortalResults(
    CaptivePortalResult previous_result,
    CaptivePortalResult result) {
  if (result != captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL)
    is_login_tab_ = false;
}

void CaptivePortalLoginDetector::SetIsLoginTab() {
  is_login_tab_ = true;
  first_login_tab_load_ = true;
}
