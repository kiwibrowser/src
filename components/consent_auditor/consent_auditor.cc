// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/consent_auditor/consent_auditor.h"

#include <memory>

#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "components/consent_auditor/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/user_events/user_event_service.h"

using sync_pb::UserConsentTypes;
using sync_pb::UserEventSpecifics;

namespace consent_auditor {

namespace {

const char kLocalConsentDescriptionKey[] = "description";
const char kLocalConsentConfirmationKey[] = "confirmation";
const char kLocalConsentVersionKey[] = "version";
const char kLocalConsentLocaleKey[] = "locale";

UserEventSpecifics::UserConsent::Feature FeatureToProtoEnum(
    consent_auditor::Feature feature) {
  switch (feature) {
    case consent_auditor::Feature::CHROME_SYNC:
      return UserEventSpecifics::UserConsent::CHROME_SYNC;
    case consent_auditor::Feature::PLAY_STORE:
      return UserEventSpecifics::UserConsent::PLAY_STORE;
    case consent_auditor::Feature::BACKUP_AND_RESTORE:
      return UserEventSpecifics::UserConsent::BACKUP_AND_RESTORE;
    case consent_auditor::Feature::GOOGLE_LOCATION_SERVICE:
      return UserEventSpecifics::UserConsent::GOOGLE_LOCATION_SERVICE;
  }
  NOTREACHED();
  return UserEventSpecifics::UserConsent::FEATURE_UNSPECIFIED;
}

UserConsentTypes::ConsentStatus StatusToProtoEnum(
    consent_auditor::ConsentStatus status) {
  switch (status) {
    case consent_auditor::ConsentStatus::NOT_GIVEN:
      return UserConsentTypes::NOT_GIVEN;
    case consent_auditor::ConsentStatus::GIVEN:
      return UserConsentTypes::GIVEN;
  }
  NOTREACHED();
  return UserConsentTypes::CONSENT_STATUS_UNSPECIFIED;
}

}  // namespace

ConsentAuditor::ConsentAuditor(PrefService* pref_service,
                               syncer::UserEventService* user_event_service,
                               const std::string& app_version,
                               const std::string& app_locale)
    : pref_service_(pref_service),
      user_event_service_(user_event_service),
      app_version_(app_version),
      app_locale_(app_locale) {
  DCHECK(pref_service_);
  DCHECK(user_event_service_);
}

ConsentAuditor::~ConsentAuditor() {}

void ConsentAuditor::Shutdown() {
  user_event_service_ = nullptr;
}

// static
void ConsentAuditor::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kLocalConsentsDictionary);
}

void ConsentAuditor::RecordGaiaConsent(
    const std::string& account_id,
    Feature feature,
    const std::vector<int>& description_grd_ids,
    int confirmation_grd_id,
    ConsentStatus status) {
  DCHECK(!account_id.empty()) << "No signed-in account specified.";

  if (!base::FeatureList::IsEnabled(switches::kSyncUserConsentEvents))
    return;

  DCHECK_LE(feature, consent_auditor::Feature::FEATURE_LAST);

  switch (status) {
    case ConsentStatus::GIVEN:
      UMA_HISTOGRAM_ENUMERATION(
          "Privacy.ConsentAuditor.ConsentGiven.Feature", feature,
          static_cast<int>(consent_auditor::Feature::FEATURE_LAST) + 1);
      break;
    case ConsentStatus::NOT_GIVEN:
      UMA_HISTOGRAM_ENUMERATION(
          "Privacy.ConsentAuditor.ConsentNotGiven.Feature", feature,
          static_cast<int>(consent_auditor::Feature::FEATURE_LAST) + 1);
      break;
  }

  // TODO(msramek): Pass in the actual account id.
  std::unique_ptr<sync_pb::UserEventSpecifics> specifics = ConstructUserConsent(
      account_id, feature, description_grd_ids, confirmation_grd_id, status);
  user_event_service_->RecordUserEvent(std::move(specifics));
}

std::unique_ptr<sync_pb::UserEventSpecifics>
ConsentAuditor::ConstructUserConsent(
    const std::string& account_id,
    Feature feature,
    const std::vector<int>& description_grd_ids,
    int confirmation_grd_id,
    ConsentStatus status) {
  auto specifics = std::make_unique<sync_pb::UserEventSpecifics>();
  specifics->set_event_time_usec(
      base::Time::Now().since_origin().InMicroseconds());
  auto* consent = specifics->mutable_user_consent();
  consent->set_account_id(account_id);
  consent->set_feature(FeatureToProtoEnum(feature));
  for (int id : description_grd_ids) {
    consent->add_description_grd_ids(id);
  }
  consent->set_confirmation_grd_id(confirmation_grd_id);
  consent->set_locale(app_locale_);
  consent->set_status(StatusToProtoEnum(status));
  return specifics;
}

void ConsentAuditor::RecordLocalConsent(const std::string& feature,
                                        const std::string& description_text,
                                        const std::string& confirmation_text) {
  DictionaryPrefUpdate consents_update(pref_service_,
                                       prefs::kLocalConsentsDictionary);
  base::DictionaryValue* consents = consents_update.Get();
  DCHECK(consents);

  base::DictionaryValue record;
  record.SetKey(kLocalConsentDescriptionKey, base::Value(description_text));
  record.SetKey(kLocalConsentConfirmationKey, base::Value(confirmation_text));
  record.SetKey(kLocalConsentVersionKey, base::Value(app_version_));
  record.SetKey(kLocalConsentLocaleKey, base::Value(app_locale_));

  consents->SetKey(feature, std::move(record));
}

}  // namespace consent_auditor
