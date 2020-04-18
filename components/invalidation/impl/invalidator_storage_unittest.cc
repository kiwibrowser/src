// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidator_storage.h"

#include "base/strings/string_util.h"
#include "components/invalidation/impl/unacked_invalidation_set_test_util.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace invalidation {

class InvalidatorStorageTest : public testing::Test {
 public:
  InvalidatorStorageTest() {}

  void SetUp() override {
    InvalidatorStorage::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

// Clearing the storage should erase all version map entries, bootstrap data,
// and the client ID.
TEST_F(InvalidatorStorageTest, Clear) {
  InvalidatorStorage storage(&pref_service_);
  EXPECT_TRUE(storage.GetBootstrapData().empty());
  EXPECT_TRUE(storage.GetInvalidatorClientId().empty());

  storage.ClearAndSetNewClientId("fake_id");
  EXPECT_EQ("fake_id", storage.GetInvalidatorClientId());

  storage.SetBootstrapData("test");
  EXPECT_EQ("test", storage.GetBootstrapData());

  storage.Clear();

  EXPECT_TRUE(storage.GetBootstrapData().empty());
  EXPECT_TRUE(storage.GetInvalidatorClientId().empty());
}

TEST_F(InvalidatorStorageTest, SetGetNotifierClientId) {
  InvalidatorStorage storage(&pref_service_);
  const std::string client_id("fK6eDzAIuKqx9A4+93bljg==");

  storage.ClearAndSetNewClientId(client_id);
  EXPECT_EQ(client_id, storage.GetInvalidatorClientId());
}

TEST_F(InvalidatorStorageTest, SetGetBootstrapData) {
  InvalidatorStorage storage(&pref_service_);
  const std::string mess("n\0tK\0\0l\344", 8);
  ASSERT_FALSE(base::IsStringUTF8(mess));

  storage.SetBootstrapData(mess);
  EXPECT_EQ(mess, storage.GetBootstrapData());
}

TEST_F(InvalidatorStorageTest, SaveGetInvalidations_Empty) {
  InvalidatorStorage storage(&pref_service_);
  syncer::UnackedInvalidationsMap empty_map;
  ASSERT_TRUE(empty_map.empty());

  storage.SetSavedInvalidations(empty_map);
  syncer::UnackedInvalidationsMap restored_map =
      storage.GetSavedInvalidations();
  EXPECT_TRUE(restored_map.empty());
}

TEST_F(InvalidatorStorageTest, SaveGetInvalidations) {
  InvalidatorStorage storage(&pref_service_);

  ObjectId id1(10, "object1");
  syncer::UnackedInvalidationSet storage1(id1);
  syncer::Invalidation unknown_version_inv =
      syncer::Invalidation::InitUnknownVersion(id1);
  syncer::Invalidation known_version_inv =
      syncer::Invalidation::Init(id1, 314, "payload");
  storage1.Add(unknown_version_inv);
  storage1.Add(known_version_inv);

  ObjectId id2(10, "object2");
  syncer::UnackedInvalidationSet storage2(id2);
  syncer::Invalidation obj2_inv =
      syncer::Invalidation::Init(id2, 1234, "payl\0ad\xff");
  storage2.Add(obj2_inv);

  syncer::UnackedInvalidationsMap map;
  map.insert(std::make_pair(storage1.object_id(), storage1));
  map.insert(std::make_pair(storage2.object_id(), storage2));

  storage.SetSavedInvalidations(map);
  syncer::UnackedInvalidationsMap restored_map =
      storage.GetSavedInvalidations();

  EXPECT_THAT(map, syncer::test_util::Eq(restored_map));
}

}  // namespace invalidation
