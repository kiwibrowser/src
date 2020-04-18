// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONSENT_AUDITOR_FAKE_CONSENT_AUDITOR_H_
#define COMPONENTS_CONSENT_AUDITOR_FAKE_CONSENT_AUDITOR_H_

#include <vector>

#include "base/macros.h"
#include "components/consent_auditor/consent_auditor.h"

namespace syncer {
class UserEventService;
}

class PrefService;

namespace consent_auditor {

class FakeConsentAuditor : public ConsentAuditor {
 public:
  FakeConsentAuditor(PrefService* pref_service,
                     syncer::UserEventService* user_event_service);
  ~FakeConsentAuditor() override;

  void RecordGaiaConsent(const std::string& account_id,
                         consent_auditor::Feature feature,
                         const std::vector<int>& description_grd_ids,
                         int confirmation_grd_id,
                         consent_auditor::ConsentStatus status) override;

  const std::string& account_id() const { return account_id_; }

  const std::vector<std::vector<int>>& recorded_id_vectors() {
    return recorded_id_vectors_;
  }

  const std::vector<Feature>& recorded_features() { return recorded_features_; }

  const std::vector<ConsentStatus>& recorded_statuses() {
    return recorded_statuses_;
  }

 private:
  std::string account_id_;
  std::vector<std::vector<int>> recorded_id_vectors_;
  std::vector<Feature> recorded_features_;
  std::vector<ConsentStatus> recorded_statuses_;

  DISALLOW_COPY_AND_ASSIGN(FakeConsentAuditor);
};

}  // namespace consent_auditor

#endif  // COMPONENTS_CONSENT_AUDITOR_FAKE_CONSENT_AUDITOR_H_
