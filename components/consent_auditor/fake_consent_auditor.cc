// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/consent_auditor/fake_consent_auditor.h"

namespace consent_auditor {

FakeConsentAuditor::FakeConsentAuditor(
    PrefService* pref_service,
    syncer::UserEventService* user_event_service)
    : ConsentAuditor(pref_service,
                     user_event_service,
                     std::string(),
                     std::string()) {}

FakeConsentAuditor::~FakeConsentAuditor() {}

void FakeConsentAuditor::RecordGaiaConsent(
    const std::string& account_id,
    consent_auditor::Feature feature,
    const std::vector<int>& description_grd_ids,
    int confirmation_grd_id,
    consent_auditor::ConsentStatus status) {
  account_id_ = account_id;
  std::vector<int> ids = description_grd_ids;
  ids.push_back(confirmation_grd_id);
  recorded_id_vectors_.push_back(std::move(ids));
  recorded_features_.push_back(feature);
  recorded_statuses_.push_back(status);
}

}  // namespace consent_auditor
