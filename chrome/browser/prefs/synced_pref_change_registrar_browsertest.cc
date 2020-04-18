// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/json/json_string_value_serializer.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/sync/model/fake_sync_change_processor.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_error_factory.h"
#include "components/sync/model/sync_error_factory_mock.h"
#include "components/sync/model/syncable_service.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_preferences/synced_pref_change_registrar.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_utils.h"

namespace {

using testing::Return;
using testing::_;

class SyncedPrefChangeRegistrarTest : public InProcessBrowserTest {
 public:
  SyncedPrefChangeRegistrarTest() : next_sync_data_id_(0) {}
  ~SyncedPrefChangeRegistrarTest() override {}

  void UpdateChromePolicy(const policy::PolicyMap& policies) {
    policy_provider_.UpdateChromePolicy(policies);
    DCHECK(base::MessageLoopCurrent::Get());
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  void SetBooleanPrefValueFromSync(const std::string& name, bool value) {
    std::string serialized_value;
    JSONStringValueSerializer json(&serialized_value);
    json.Serialize(base::Value(value));

    sync_pb::EntitySpecifics specifics;
    sync_pb::PreferenceSpecifics* pref_specifics =
        specifics.mutable_preference();
    pref_specifics->set_name(name);
    pref_specifics->set_value(serialized_value);

    syncer::SyncData change_data = syncer::SyncData::CreateRemoteData(
        ++next_sync_data_id_, specifics, base::Time());
    syncer::SyncChange change(
        FROM_HERE, syncer::SyncChange::ACTION_UPDATE, change_data);

    syncer::SyncChangeList change_list;
    change_list.push_back(change);

    syncer_->ProcessSyncChanges(FROM_HERE, change_list);
  }

  void SetBooleanPrefValueFromLocal(const std::string& name, bool value) {
    prefs_->SetBoolean(name.c_str(), value);
  }

  bool GetBooleanPrefValue(const std::string& name) {
    return prefs_->GetBoolean(name.c_str());
  }

  sync_preferences::PrefServiceSyncable* prefs() const { return prefs_; }

  sync_preferences::SyncedPrefChangeRegistrar* registrar() const {
    return registrar_.get();
  }

 private:
  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(policy_provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
        &policy_provider_);
  }

  void SetUpOnMainThread() override {
    prefs_ = PrefServiceSyncableFromProfile(browser()->profile());
    syncer_ = prefs_->GetSyncableService(syncer::PREFERENCES);
    syncer_->MergeDataAndStartSyncing(
        syncer::PREFERENCES, syncer::SyncDataList(),
        std::unique_ptr<syncer::SyncChangeProcessor>(
            new syncer::FakeSyncChangeProcessor),
        std::unique_ptr<syncer::SyncErrorFactory>(
            new syncer::SyncErrorFactoryMock));
    registrar_.reset(new sync_preferences::SyncedPrefChangeRegistrar(prefs_));
  }

  void TearDownOnMainThread() override { registrar_.reset(); }

  sync_preferences::PrefServiceSyncable* prefs_;
  syncer::SyncableService* syncer_;
  int next_sync_data_id_;

  std::unique_ptr<sync_preferences::SyncedPrefChangeRegistrar> registrar_;
  policy::MockConfigurationPolicyProvider policy_provider_;
};

struct TestSyncedPrefObserver {
  bool last_seen_value;
  bool last_update_is_from_sync;
  bool has_been_notified;
};

void TestPrefChangeCallback(PrefService* prefs,
                            TestSyncedPrefObserver* observer,
                            const std::string& path,
                            bool from_sync) {
  observer->last_seen_value = prefs->GetBoolean(path.c_str());
  observer->last_update_is_from_sync = from_sync;
  observer->has_been_notified = true;
}

}  // namespace

IN_PROC_BROWSER_TEST_F(SyncedPrefChangeRegistrarTest,
                       DifferentiateRemoteAndLocalChanges) {
  TestSyncedPrefObserver observer = {};
  registrar()->Add(prefs::kShowHomeButton,
      base::Bind(&TestPrefChangeCallback, prefs(), &observer));

  EXPECT_FALSE(observer.has_been_notified);

  SetBooleanPrefValueFromSync(prefs::kShowHomeButton, true);
  EXPECT_TRUE(observer.has_been_notified);
  EXPECT_TRUE(GetBooleanPrefValue(prefs::kShowHomeButton));
  EXPECT_TRUE(observer.last_update_is_from_sync);
  EXPECT_TRUE(observer.last_seen_value);

  observer.has_been_notified = false;
  SetBooleanPrefValueFromLocal(prefs::kShowHomeButton, false);
  EXPECT_TRUE(observer.has_been_notified);
  EXPECT_FALSE(GetBooleanPrefValue(prefs::kShowHomeButton));
  EXPECT_FALSE(observer.last_update_is_from_sync);
  EXPECT_FALSE(observer.last_seen_value);

  observer.has_been_notified = false;
  SetBooleanPrefValueFromLocal(prefs::kShowHomeButton, true);
  EXPECT_TRUE(observer.has_been_notified);
  EXPECT_TRUE(GetBooleanPrefValue(prefs::kShowHomeButton));
  EXPECT_FALSE(observer.last_update_is_from_sync);
  EXPECT_TRUE(observer.last_seen_value);

  observer.has_been_notified = false;
  SetBooleanPrefValueFromSync(prefs::kShowHomeButton, false);
  EXPECT_TRUE(observer.has_been_notified);
  EXPECT_FALSE(GetBooleanPrefValue(prefs::kShowHomeButton));
  EXPECT_TRUE(observer.last_update_is_from_sync);
  EXPECT_FALSE(observer.last_seen_value);
}

IN_PROC_BROWSER_TEST_F(SyncedPrefChangeRegistrarTest,
                       IgnoreLocalChangesToManagedPrefs) {
  TestSyncedPrefObserver observer = {};
  registrar()->Add(prefs::kShowHomeButton,
      base::Bind(&TestPrefChangeCallback, prefs(), &observer));

  policy::PolicyMap policies;
  policies.Set(policy::key::kShowHomeButton, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
               std::make_unique<base::Value>(true), nullptr);
  UpdateChromePolicy(policies);

  EXPECT_TRUE(prefs()->IsManagedPreference(prefs::kShowHomeButton));

  SetBooleanPrefValueFromLocal(prefs::kShowHomeButton, false);
  EXPECT_FALSE(observer.has_been_notified);
  EXPECT_TRUE(GetBooleanPrefValue(prefs::kShowHomeButton));
}

IN_PROC_BROWSER_TEST_F(SyncedPrefChangeRegistrarTest,
                       IgnoreSyncChangesToManagedPrefs) {
  TestSyncedPrefObserver observer = {};
  registrar()->Add(prefs::kShowHomeButton,
      base::Bind(&TestPrefChangeCallback, prefs(), &observer));

  policy::PolicyMap policies;
  policies.Set(policy::key::kShowHomeButton, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
               std::make_unique<base::Value>(true), nullptr);
  UpdateChromePolicy(policies);

  EXPECT_TRUE(prefs()->IsManagedPreference(prefs::kShowHomeButton));
  SetBooleanPrefValueFromSync(prefs::kShowHomeButton, false);
  EXPECT_FALSE(observer.has_been_notified);
  EXPECT_TRUE(GetBooleanPrefValue(prefs::kShowHomeButton));
}
