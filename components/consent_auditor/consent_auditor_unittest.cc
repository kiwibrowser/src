// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/consent_auditor/consent_auditor.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "components/consent_auditor/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/user_events/fake_user_event_service.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::UserEventSpecifics;

namespace consent_auditor {

namespace {

const char kLocalConsentDescriptionKey[] = "description";
const char kLocalConsentConfirmationKey[] = "confirmation";
const char kLocalConsentVersionKey[] = "version";
const char kLocalConsentLocaleKey[] = "locale";

// Fake product version for testing.
const char kCurrentAppVersion[] = "1.2.3.4";
const char kCurrentAppLocale[] = "en-US";

// Fake account ID for testing.
const char kAccountId[] = "testing_account_id";

// A helper function to load the |description|, |confirmation|, |version|,
// and |locale|, in that order, from a record for the |feature| in
// the |consents| dictionary.
void LoadEntriesFromLocalConsentRecord(const base::Value* consents,
                                       const std::string& feature,
                                       std::string* description,
                                       std::string* confirmation,
                                       std::string* version,
                                       std::string* locale) {
  SCOPED_TRACE(::testing::Message() << "|feature| = " << feature);

  const base::Value* record =
      consents->FindKeyOfType(feature, base::Value::Type::DICTIONARY);
  ASSERT_TRUE(record);
  SCOPED_TRACE(::testing::Message() << "|record| = " << record);

  const base::Value* description_entry =
      record->FindKey(kLocalConsentDescriptionKey);
  const base::Value* confirmation_entry =
      record->FindKey(kLocalConsentConfirmationKey);
  const base::Value* version_entry = record->FindKey(kLocalConsentVersionKey);
  const base::Value* locale_entry = record->FindKey(kLocalConsentLocaleKey);

  ASSERT_TRUE(description_entry);
  ASSERT_TRUE(confirmation_entry);
  ASSERT_TRUE(version_entry);
  ASSERT_TRUE(locale_entry);

  *description = description_entry->GetString();
  *confirmation = confirmation_entry->GetString();
  *version = version_entry->GetString();
  *locale = locale_entry->GetString();
}

}  // namespace

class ConsentAuditorTest : public testing::Test {
 public:
  void SetUp() override {
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    user_event_service_ = std::make_unique<syncer::FakeUserEventService>();
    ConsentAuditor::RegisterProfilePrefs(pref_service_->registry());
    consent_auditor_ = std::make_unique<ConsentAuditor>(
        pref_service_.get(), user_event_service_.get(), kCurrentAppVersion,
        kCurrentAppLocale);
  }

  void UpdateAppVersionAndLocale(const std::string& new_product_version,
                                 const std::string& new_app_locale) {
    // We'll have to recreate |consent_auditor| in order to update the version
    // and locale. This is not a problem, as in reality we'd have to restart
    // Chrome to update both, let alone just recreate this class.
    consent_auditor_ = std::make_unique<ConsentAuditor>(
        pref_service_.get(), user_event_service_.get(), new_product_version,
        new_app_locale);
  }

  ConsentAuditor* consent_auditor() { return consent_auditor_.get(); }

  PrefService* pref_service() const { return pref_service_.get(); }

  syncer::FakeUserEventService* user_event_service() {
    return user_event_service_.get();
  }

 private:
  std::unique_ptr<ConsentAuditor> consent_auditor_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<syncer::FakeUserEventService> user_event_service_;
};

TEST_F(ConsentAuditorTest, LocalConsentPrefRepresentation) {
  // No consents are written at first.
  EXPECT_FALSE(pref_service()->HasPrefPath(prefs::kLocalConsentsDictionary));

  // Record a consent and check that it appears in the prefs.
  const std::string kFeature1Description = "This will enable feature 1.";
  const std::string kFeature1Confirmation = "OK.";
  consent_auditor()->RecordLocalConsent("feature1", kFeature1Description,
                                        kFeature1Confirmation);
  ASSERT_TRUE(pref_service()->HasPrefPath(prefs::kLocalConsentsDictionary));
  const base::DictionaryValue* consents =
      pref_service()->GetDictionary(prefs::kLocalConsentsDictionary);
  ASSERT_TRUE(consents);
  std::string description;
  std::string confirmation;
  std::string version;
  std::string locale;
  LoadEntriesFromLocalConsentRecord(consents, "feature1", &description,
                                    &confirmation, &version, &locale);
  EXPECT_EQ(kFeature1Description, description);
  EXPECT_EQ(kFeature1Confirmation, confirmation);
  EXPECT_EQ(kCurrentAppVersion, version);
  EXPECT_EQ(kCurrentAppLocale, locale);

  // Do the same for another feature.
  const std::string kFeature2Description = "Enable feature 2?";
  const std::string kFeature2Confirmation = "Yes.";
  consent_auditor()->RecordLocalConsent("feature2", kFeature2Description,
                                        kFeature2Confirmation);
  LoadEntriesFromLocalConsentRecord(consents, "feature2", &description,
                                    &confirmation, &version, &locale);
  EXPECT_EQ(kFeature2Description, description);
  EXPECT_EQ(kFeature2Confirmation, confirmation);
  EXPECT_EQ(kCurrentAppVersion, version);
  EXPECT_EQ(kCurrentAppLocale, locale);

  // They are two separate records; the latter did not overwrite the former.
  EXPECT_EQ(2u, consents->size());
  EXPECT_TRUE(
      consents->FindKeyOfType("feature1", base::Value::Type::DICTIONARY));

  // Overwrite an existing consent, this time use a different product version
  // and a different locale.
  const std::string kFeature2NewDescription = "Re-enable feature 2?";
  const std::string kFeature2NewConfirmation = "Yes again.";
  const std::string kFeature2NewAppVersion = "5.6.7.8";
  const std::string kFeature2NewAppLocale = "de";
  UpdateAppVersionAndLocale(kFeature2NewAppVersion, kFeature2NewAppLocale);
  consent_auditor()->RecordLocalConsent("feature2", kFeature2NewDescription,
                                        kFeature2NewConfirmation);
  LoadEntriesFromLocalConsentRecord(consents, "feature2", &description,
                                    &confirmation, &version, &locale);
  EXPECT_EQ(kFeature2NewDescription, description);
  EXPECT_EQ(kFeature2NewConfirmation, confirmation);
  EXPECT_EQ(kFeature2NewAppVersion, version);
  EXPECT_EQ(kFeature2NewAppLocale, locale);

  // We still have two records.
  EXPECT_EQ(2u, consents->size());
}

TEST_F(ConsentAuditorTest, RecordingEnabled) {
  consent_auditor()->RecordGaiaConsent(kAccountId, Feature::CHROME_SYNC, {}, 0,
                                       ConsentStatus::GIVEN);
  auto& events = user_event_service()->GetRecordedUserEvents();
  EXPECT_EQ(1U, events.size());
}

TEST_F(ConsentAuditorTest, RecordingDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(switches::kSyncUserConsentEvents);
  consent_auditor()->RecordGaiaConsent(kAccountId, Feature::CHROME_SYNC, {}, 0,
                                       ConsentStatus::GIVEN);
  auto& events = user_event_service()->GetRecordedUserEvents();
  EXPECT_EQ(0U, events.size());
}

TEST_F(ConsentAuditorTest, RecordGaiaConsent) {
  std::vector<int> kDescriptionMessageIds = {12, 37, 42};
  int kConfirmationMessageId = 47;
  base::Time t1 = base::Time::Now();
  consent_auditor()->RecordGaiaConsent(
      kAccountId, Feature::CHROME_SYNC, kDescriptionMessageIds,
      kConfirmationMessageId, ConsentStatus::GIVEN);
  base::Time t2 = base::Time::Now();
  auto& events = user_event_service()->GetRecordedUserEvents();
  EXPECT_EQ(1U, events.size());
  EXPECT_LE(t1.since_origin().InMicroseconds(), events[0].event_time_usec());
  EXPECT_GE(t2.since_origin().InMicroseconds(), events[0].event_time_usec());
  EXPECT_FALSE(events[0].has_navigation_id());
  EXPECT_TRUE(events[0].has_user_consent());
  auto& consent = events[0].user_consent();
  EXPECT_EQ(kAccountId, consent.account_id());
  EXPECT_EQ(UserEventSpecifics::UserConsent::CHROME_SYNC, consent.feature());
  EXPECT_EQ(3, consent.description_grd_ids_size());
  EXPECT_EQ(kDescriptionMessageIds[0], consent.description_grd_ids(0));
  EXPECT_EQ(kDescriptionMessageIds[1], consent.description_grd_ids(1));
  EXPECT_EQ(kDescriptionMessageIds[2], consent.description_grd_ids(2));
  EXPECT_EQ(kConfirmationMessageId, consent.confirmation_grd_id());
  EXPECT_EQ(kCurrentAppLocale, consent.locale());
}

}  // namespace consent_auditor
