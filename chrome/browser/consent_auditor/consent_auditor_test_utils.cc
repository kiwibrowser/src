// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/consent_auditor/consent_auditor_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "components/consent_auditor/fake_consent_auditor.h"

std::unique_ptr<KeyedService> BuildFakeConsentAuditor(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  std::unique_ptr<consent_auditor::FakeConsentAuditor> fake_consent_auditor =
      std::make_unique<consent_auditor::FakeConsentAuditor>(
          profile->GetPrefs(),
          browser_sync::UserEventServiceFactory::GetForProfile(profile));
  return fake_consent_auditor;
}
