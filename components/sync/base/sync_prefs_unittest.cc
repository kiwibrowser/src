// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/sync_prefs.h"

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/pref_value_store.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using ::testing::InSequence;
using ::testing::StrictMock;

class SyncPrefsTest : public testing::Test {
 protected:
  void SetUp() override {
    SyncPrefs::RegisterProfilePrefs(pref_service_.registry());
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;

 private:
  base::MessageLoop loop_;
};

TEST_F(SyncPrefsTest, Basic) {
  SyncPrefs sync_prefs(&pref_service_);

  EXPECT_FALSE(sync_prefs.IsFirstSetupComplete());
  sync_prefs.SetFirstSetupComplete();
  EXPECT_TRUE(sync_prefs.IsFirstSetupComplete());

  EXPECT_TRUE(sync_prefs.IsSyncRequested());
  sync_prefs.SetSyncRequested(false);
  EXPECT_FALSE(sync_prefs.IsSyncRequested());
  sync_prefs.SetSyncRequested(true);
  EXPECT_TRUE(sync_prefs.IsSyncRequested());

  EXPECT_EQ(base::Time(), sync_prefs.GetLastSyncedTime());
  const base::Time& now = base::Time::Now();
  sync_prefs.SetLastSyncedTime(now);
  EXPECT_EQ(now, sync_prefs.GetLastSyncedTime());

  EXPECT_TRUE(sync_prefs.HasKeepEverythingSynced());
  sync_prefs.SetKeepEverythingSynced(false);
  EXPECT_FALSE(sync_prefs.HasKeepEverythingSynced());
  sync_prefs.SetKeepEverythingSynced(true);
  EXPECT_TRUE(sync_prefs.HasKeepEverythingSynced());

  EXPECT_TRUE(sync_prefs.GetEncryptionBootstrapToken().empty());
  sync_prefs.SetEncryptionBootstrapToken("token");
  EXPECT_EQ("token", sync_prefs.GetEncryptionBootstrapToken());
}

TEST_F(SyncPrefsTest, DefaultTypes) {
  SyncPrefs sync_prefs(&pref_service_);
  sync_prefs.SetKeepEverythingSynced(false);

  // Only device info is enabled by default.
  ModelTypeSet expected(DEVICE_INFO);
  ModelTypeSet preferred_types = sync_prefs.GetPreferredDataTypes(UserTypes());
  EXPECT_EQ(expected, preferred_types);

  // Simulate an upgrade to delete directives + proxy tabs support. None of the
  // new types or their pref group types should be registering, ensuring they
  // don't have pref values.
  ModelTypeSet registered_types = UserTypes();
  registered_types.Remove(PROXY_TABS);
  registered_types.Remove(TYPED_URLS);
  registered_types.Remove(SESSIONS);
  registered_types.Remove(HISTORY_DELETE_DIRECTIVES);

  // Enable all other types.
  sync_prefs.SetPreferredDataTypes(registered_types, registered_types);

  // Manually enable typed urls (to simulate the old world).
  pref_service_.SetBoolean(prefs::kSyncTypedUrls, true);

  // Proxy tabs should not be enabled (since sessions wasn't), but history
  // delete directives should (since typed urls was).
  preferred_types = sync_prefs.GetPreferredDataTypes(UserTypes());
  EXPECT_FALSE(preferred_types.Has(PROXY_TABS));
  EXPECT_TRUE(preferred_types.Has(HISTORY_DELETE_DIRECTIVES));

  // Now manually enable sessions, which should result in proxy tabs also being
  // enabled. Also, manually disable typed urls, which should mean that history
  // delete directives are not enabled.
  pref_service_.SetBoolean(prefs::kSyncTypedUrls, false);
  pref_service_.SetBoolean(prefs::kSyncSessions, true);
  preferred_types = sync_prefs.GetPreferredDataTypes(UserTypes());
  EXPECT_TRUE(preferred_types.Has(PROXY_TABS));
  EXPECT_FALSE(preferred_types.Has(HISTORY_DELETE_DIRECTIVES));
}

TEST_F(SyncPrefsTest, PreferredTypesKeepEverythingSynced) {
  SyncPrefs sync_prefs(&pref_service_);

  EXPECT_TRUE(sync_prefs.HasKeepEverythingSynced());

  const ModelTypeSet user_types = UserTypes();
  EXPECT_EQ(user_types, sync_prefs.GetPreferredDataTypes(user_types));
  const ModelTypeSet user_visible_types = UserSelectableTypes();
  for (ModelTypeSet::Iterator it = user_visible_types.First(); it.Good();
       it.Inc()) {
    ModelTypeSet preferred_types;
    preferred_types.Put(it.Get());
    sync_prefs.SetPreferredDataTypes(user_types, preferred_types);
    EXPECT_EQ(user_types, sync_prefs.GetPreferredDataTypes(user_types));
  }
}

TEST_F(SyncPrefsTest, PreferredTypesNotKeepEverythingSynced) {
  SyncPrefs sync_prefs(&pref_service_);

  sync_prefs.SetKeepEverythingSynced(false);

  const ModelTypeSet user_types = UserTypes();
  EXPECT_NE(user_types, sync_prefs.GetPreferredDataTypes(user_types));
  const ModelTypeSet user_visible_types = UserSelectableTypes();
  for (ModelTypeSet::Iterator it = user_visible_types.First(); it.Good();
       it.Inc()) {
    ModelTypeSet preferred_types;
    preferred_types.Put(it.Get());
    ModelTypeSet expected_preferred_types(preferred_types);
    if (it.Get() == AUTOFILL) {
      expected_preferred_types.Put(AUTOFILL_PROFILE);
      expected_preferred_types.Put(AUTOFILL_WALLET_DATA);
      expected_preferred_types.Put(AUTOFILL_WALLET_METADATA);
    }
    if (it.Get() == PREFERENCES) {
      expected_preferred_types.Put(DICTIONARY);
      expected_preferred_types.Put(PRIORITY_PREFERENCES);
      expected_preferred_types.Put(SEARCH_ENGINES);
    }
    if (it.Get() == APPS) {
      expected_preferred_types.Put(APP_LIST);
      expected_preferred_types.Put(APP_NOTIFICATIONS);
      expected_preferred_types.Put(APP_SETTINGS);
      expected_preferred_types.Put(ARC_PACKAGE);
      expected_preferred_types.Put(READING_LIST);
    }
    if (it.Get() == EXTENSIONS) {
      expected_preferred_types.Put(EXTENSION_SETTINGS);
    }
    if (it.Get() == TYPED_URLS) {
      expected_preferred_types.Put(HISTORY_DELETE_DIRECTIVES);
      expected_preferred_types.Put(SESSIONS);
      expected_preferred_types.Put(FAVICON_IMAGES);
      expected_preferred_types.Put(FAVICON_TRACKING);
      expected_preferred_types.Put(USER_EVENTS);
    }
    if (it.Get() == PROXY_TABS) {
      expected_preferred_types.Put(SESSIONS);
      expected_preferred_types.Put(FAVICON_IMAGES);
      expected_preferred_types.Put(FAVICON_TRACKING);
    }

    // Device info is always preferred.
    expected_preferred_types.Put(DEVICE_INFO);

    sync_prefs.SetPreferredDataTypes(user_types, preferred_types);
    EXPECT_EQ(expected_preferred_types,
              sync_prefs.GetPreferredDataTypes(user_types));
  }
}

class MockSyncPrefObserver : public SyncPrefObserver {
 public:
  MOCK_METHOD1(OnSyncManagedPrefChange, void(bool));
};

TEST_F(SyncPrefsTest, ObservedPrefs) {
  SyncPrefs sync_prefs(&pref_service_);

  StrictMock<MockSyncPrefObserver> mock_sync_pref_observer;
  InSequence dummy;
  EXPECT_CALL(mock_sync_pref_observer, OnSyncManagedPrefChange(true));
  EXPECT_CALL(mock_sync_pref_observer, OnSyncManagedPrefChange(false));

  EXPECT_FALSE(sync_prefs.IsManaged());

  sync_prefs.AddSyncPrefObserver(&mock_sync_pref_observer);

  sync_prefs.SetManagedForTest(true);
  EXPECT_TRUE(sync_prefs.IsManaged());
  sync_prefs.SetManagedForTest(false);
  EXPECT_FALSE(sync_prefs.IsManaged());

  sync_prefs.RemoveSyncPrefObserver(&mock_sync_pref_observer);
}

TEST_F(SyncPrefsTest, ClearPreferences) {
  SyncPrefs sync_prefs(&pref_service_);

  EXPECT_FALSE(sync_prefs.IsFirstSetupComplete());
  EXPECT_EQ(base::Time(), sync_prefs.GetLastSyncedTime());
  EXPECT_TRUE(sync_prefs.GetEncryptionBootstrapToken().empty());

  sync_prefs.SetFirstSetupComplete();
  sync_prefs.SetLastSyncedTime(base::Time::Now());
  sync_prefs.SetEncryptionBootstrapToken("token");

  EXPECT_TRUE(sync_prefs.IsFirstSetupComplete());
  EXPECT_NE(base::Time(), sync_prefs.GetLastSyncedTime());
  EXPECT_EQ("token", sync_prefs.GetEncryptionBootstrapToken());

  sync_prefs.ClearPreferences();

  EXPECT_FALSE(sync_prefs.IsFirstSetupComplete());
  EXPECT_EQ(base::Time(), sync_prefs.GetLastSyncedTime());
  EXPECT_TRUE(sync_prefs.GetEncryptionBootstrapToken().empty());
}

// Device info should always be enabled.
TEST_F(SyncPrefsTest, DeviceInfo) {
  SyncPrefs sync_prefs(&pref_service_);
  EXPECT_TRUE(sync_prefs.GetPreferredDataTypes(UserTypes()).Has(DEVICE_INFO));
  sync_prefs.SetKeepEverythingSynced(true);
  EXPECT_TRUE(sync_prefs.GetPreferredDataTypes(UserTypes()).Has(DEVICE_INFO));
  sync_prefs.SetKeepEverythingSynced(false);
  EXPECT_TRUE(sync_prefs.GetPreferredDataTypes(UserTypes()).Has(DEVICE_INFO));
}

// Verify that invalidation versions are persisted and loaded correctly.
TEST_F(SyncPrefsTest, InvalidationVersions) {
  std::map<ModelType, int64_t> versions;
  versions[BOOKMARKS] = 10;
  versions[SESSIONS] = 20;
  versions[PREFERENCES] = 30;

  SyncPrefs sync_prefs(&pref_service_);
  sync_prefs.UpdateInvalidationVersions(versions);

  std::map<ModelType, int64_t> versions2;
  sync_prefs.GetInvalidationVersions(&versions2);

  EXPECT_EQ(versions.size(), versions2.size());
  for (auto map_iter : versions2) {
    EXPECT_EQ(versions[map_iter.first], map_iter.second);
  }
}

TEST_F(SyncPrefsTest, ShortPollInterval) {
  SyncPrefs sync_prefs(&pref_service_);
  EXPECT_TRUE(sync_prefs.GetShortPollInterval().is_zero());

  sync_prefs.SetShortPollInterval(base::TimeDelta::FromMinutes(30));

  EXPECT_FALSE(sync_prefs.GetShortPollInterval().is_zero());
  EXPECT_EQ(sync_prefs.GetShortPollInterval().InMinutes(), 30);
}

TEST_F(SyncPrefsTest, LongPollInterval) {
  SyncPrefs sync_prefs(&pref_service_);
  EXPECT_TRUE(sync_prefs.GetLongPollInterval().is_zero());

  sync_prefs.SetLongPollInterval(base::TimeDelta::FromMinutes(60));

  EXPECT_FALSE(sync_prefs.GetLongPollInterval().is_zero());
  EXPECT_EQ(sync_prefs.GetLongPollInterval().InMinutes(), 60);
}

}  // namespace

}  // namespace syncer
