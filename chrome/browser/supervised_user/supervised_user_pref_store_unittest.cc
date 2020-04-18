// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "chrome/browser/net/safe_search_util.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/browser/supervised_user/supervised_user_pref_store.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/testing_pref_store.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::DictionaryValue;
using base::Value;

namespace {

class SupervisedUserPrefStoreFixture : public PrefStore::Observer {
 public:
  explicit SupervisedUserPrefStoreFixture(
      SupervisedUserSettingsService* settings_service);
  ~SupervisedUserPrefStoreFixture() override;

  base::DictionaryValue* changed_prefs() {
    return &changed_prefs_;
  }

  bool initialization_completed() const {
    return initialization_completed_;
  }

  // PrefStore::Observer implementation:
  void OnPrefValueChanged(const std::string& key) override;
  void OnInitializationCompleted(bool succeeded) override;

 private:
  scoped_refptr<SupervisedUserPrefStore> pref_store_;
  base::DictionaryValue changed_prefs_;
  bool initialization_completed_;
};

SupervisedUserPrefStoreFixture::SupervisedUserPrefStoreFixture(
    SupervisedUserSettingsService* settings_service)
    : pref_store_(new SupervisedUserPrefStore(settings_service)),
      initialization_completed_(pref_store_->IsInitializationComplete()) {
  pref_store_->AddObserver(this);
}

SupervisedUserPrefStoreFixture::~SupervisedUserPrefStoreFixture() {
  pref_store_->RemoveObserver(this);
}

void SupervisedUserPrefStoreFixture::OnPrefValueChanged(
    const std::string& key) {
  const base::Value* value = NULL;
  ASSERT_TRUE(pref_store_->GetValue(key, &value));
  changed_prefs_.Set(key, std::make_unique<base::Value>(value->Clone()));
}

void SupervisedUserPrefStoreFixture::OnInitializationCompleted(bool succeeded) {
  EXPECT_FALSE(initialization_completed_);
  EXPECT_TRUE(succeeded);
  EXPECT_TRUE(pref_store_->IsInitializationComplete());
  initialization_completed_ = true;
}

}  // namespace

class SupervisedUserPrefStoreTest : public ::testing::Test {
 public:
  SupervisedUserPrefStoreTest() : service_(nullptr) {}
  void SetUp() override;
  void TearDown() override;

 protected:
  SupervisedUserSettingsService service_;
  scoped_refptr<TestingPrefStore> pref_store_;
};

void SupervisedUserPrefStoreTest::SetUp() {
  pref_store_ = new TestingPrefStore();
  service_.Init(pref_store_);
}

void SupervisedUserPrefStoreTest::TearDown() {
  service_.Shutdown();
}

TEST_F(SupervisedUserPrefStoreTest, ConfigureSettings) {
  SupervisedUserPrefStoreFixture fixture(&service_);
  EXPECT_FALSE(fixture.initialization_completed());

  // Prefs should not change yet when the service is ready, but not
  // activated yet.
  pref_store_->SetInitializationCompleted();
  EXPECT_TRUE(fixture.initialization_completed());
  EXPECT_EQ(0u, fixture.changed_prefs()->size());

  service_.SetActive(true);

  // kAllowDeletingBrowserHistory is hardcoded to false for supervised users.
  bool allow_deleting_browser_history = true;
  EXPECT_TRUE(fixture.changed_prefs()->GetBoolean(
      prefs::kAllowDeletingBrowserHistory, &allow_deleting_browser_history));
  EXPECT_FALSE(allow_deleting_browser_history);

  // kSupervisedModeManualHosts does not have a hardcoded value.
  base::DictionaryValue* manual_hosts = NULL;
  EXPECT_FALSE(fixture.changed_prefs()->GetDictionary(
      prefs::kSupervisedUserManualHosts, &manual_hosts));

  // kForceGoogleSafeSearch defaults to true and kForceYouTubeRestrict defaults
  // to Moderate for supervised users.
  bool force_google_safesearch = false;
  int force_youtube_restrict = safe_search_util::YOUTUBE_RESTRICT_OFF;
  EXPECT_TRUE(fixture.changed_prefs()->GetBoolean(prefs::kForceGoogleSafeSearch,
                                                  &force_google_safesearch));
  EXPECT_TRUE(fixture.changed_prefs()->GetInteger(prefs::kForceYouTubeRestrict,
                                                  &force_youtube_restrict));
  EXPECT_TRUE(force_google_safesearch);
  EXPECT_EQ(force_youtube_restrict,
            safe_search_util::YOUTUBE_RESTRICT_MODERATE);

  // Activating the service again should not change anything.
  fixture.changed_prefs()->Clear();
  service_.SetActive(true);
  EXPECT_EQ(0u, fixture.changed_prefs()->size());

  // kSupervisedModeManualHosts can be configured by the custodian.
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetBoolean("example.com", true);
  dict->SetBoolean("moose.org", false);
  service_.SetLocalSetting(supervised_users::kContentPackManualBehaviorHosts,
                           std::unique_ptr<base::Value>(dict->DeepCopy()));
  EXPECT_EQ(1u, fixture.changed_prefs()->size());
  ASSERT_TRUE(fixture.changed_prefs()->GetDictionary(
      prefs::kSupervisedUserManualHosts, &manual_hosts));
  EXPECT_TRUE(manual_hosts->Equals(dict.get()));

  // kForceGoogleSafeSearch and kForceYouTubeRestrict can be configured by the
  // custodian, overriding the hardcoded default.
  fixture.changed_prefs()->Clear();
  service_.SetLocalSetting(
      supervised_users::kForceSafeSearch,
      std::unique_ptr<base::Value>(new base::Value(false)));
  EXPECT_EQ(1u, fixture.changed_prefs()->size());
  EXPECT_TRUE(fixture.changed_prefs()->GetBoolean(prefs::kForceGoogleSafeSearch,
                                                  &force_google_safesearch));
  EXPECT_TRUE(fixture.changed_prefs()->GetInteger(prefs::kForceYouTubeRestrict,
                                                  &force_youtube_restrict));
  EXPECT_FALSE(force_google_safesearch);
  EXPECT_EQ(force_youtube_restrict, safe_search_util::YOUTUBE_RESTRICT_OFF);
}

TEST_F(SupervisedUserPrefStoreTest, ActivateSettingsBeforeInitialization) {
  SupervisedUserPrefStoreFixture fixture(&service_);
  EXPECT_FALSE(fixture.initialization_completed());

  service_.SetActive(true);
  EXPECT_FALSE(fixture.initialization_completed());
  EXPECT_EQ(0u, fixture.changed_prefs()->size());

  pref_store_->SetInitializationCompleted();
  EXPECT_TRUE(fixture.initialization_completed());
  EXPECT_EQ(0u, fixture.changed_prefs()->size());
}

TEST_F(SupervisedUserPrefStoreTest, CreatePrefStoreAfterInitialization) {
  pref_store_->SetInitializationCompleted();
  service_.SetActive(true);

  SupervisedUserPrefStoreFixture fixture(&service_);
  EXPECT_TRUE(fixture.initialization_completed());
  EXPECT_EQ(0u, fixture.changed_prefs()->size());
}

