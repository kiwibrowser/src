// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/internal/persistent_availability_store.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/test/scoped_feature_list.h"
#include "components/feature_engagement/internal/proto/availability.pb.h"
#include "components/feature_engagement/public/feature_list.h"
#include "components/leveldb_proto/proto_database.h"
#include "components/leveldb_proto/testing/fake_db.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {
const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureQux{"test_qux",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureNop{"test_nop",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

Availability CreateAvailability(const base::Feature& feature, uint32_t day) {
  Availability availability;
  availability.set_feature_name(feature.name);
  availability.set_day(day);
  return availability;
}

class PersistentAvailabilityStoreTest : public testing::Test {
 public:
  PersistentAvailabilityStoreTest()
      : db_(nullptr),
        storage_dir_(FILE_PATH_LITERAL("/persistent/store/lalala")) {
    load_callback_ = base::Bind(&PersistentAvailabilityStoreTest::LoadCallback,
                                base::Unretained(this));
  }

  ~PersistentAvailabilityStoreTest() override = default;

  // Creates a DB and stores off a pointer to it as a member.
  std::unique_ptr<leveldb_proto::test::FakeDB<Availability>> CreateDB() {
    auto db = std::make_unique<leveldb_proto::test::FakeDB<Availability>>(
        &db_availabilities_);
    db_ = db.get();
    return db;
  }

  void LoadCallback(
      bool success,
      std::unique_ptr<std::map<std::string, uint32_t>> availabilities) {
    load_successful_ = success;
    load_results_ = std::move(availabilities);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;

  // The end result of the store pipeline.
  PersistentAvailabilityStore::OnLoadedCallback load_callback_;

  // Callback results.
  base::Optional<bool> load_successful_;
  std::unique_ptr<std::map<std::string, uint32_t>> load_results_;

  // |db_availabilities_| is used during creation of the FakeDB in CreateDB(),
  // to simplify what the DB has stored.
  std::map<std::string, Availability> db_availabilities_;

  // The database that is in use.
  leveldb_proto::test::FakeDB<Availability>* db_;

  // Constant test data.
  base::FilePath storage_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistentAvailabilityStoreTest);
};

}  // namespace

TEST_F(PersistentAvailabilityStoreTest, StorageDirectory) {
  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);
  db_->InitCallback(true);
  EXPECT_EQ(storage_dir_, db_->GetDirectory());

  // Finish the pipeline to ensure the test does not leak anything.
  db_->LoadCallback(false);
}

TEST_F(PersistentAvailabilityStoreTest, InitFail) {
  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);

  db_->InitCallback(false);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_FALSE(load_successful_.value());
  EXPECT_EQ(0u, load_results_->size());
  EXPECT_EQ(0u, db_availabilities_.size());
}

TEST_F(PersistentAvailabilityStoreTest, LoadFail) {
  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(false);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_FALSE(load_successful_.value());
  EXPECT_EQ(0u, load_results_->size());
  EXPECT_EQ(0u, db_availabilities_.size());
}

TEST_F(PersistentAvailabilityStoreTest, EmptyDBEmptyFeatureFilterUpdateFailed) {
  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->UpdateCallback(false);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_FALSE(load_successful_.value());
  EXPECT_EQ(0u, load_results_->size());
  EXPECT_EQ(0u, db_availabilities_.size());
}

TEST_F(PersistentAvailabilityStoreTest, EmptyDBEmptyFeatureFilterUpdateOK) {
  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->UpdateCallback(true);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_TRUE(load_successful_.value());
  EXPECT_EQ(0u, load_results_->size());
  EXPECT_EQ(0u, db_availabilities_.size());
}

TEST_F(PersistentAvailabilityStoreTest, AllNewFeatures) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo, kTestFeatureBar},
                                        {kTestFeatureQux});

  FeatureVector feature_filter;
  feature_filter.push_back(&kTestFeatureFoo);  // Enabled. Not in DB.
  feature_filter.push_back(&kTestFeatureBar);  // Enabled. Not in DB.
  feature_filter.push_back(&kTestFeatureQux);  // Disabled. Not in DB.

  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), feature_filter, std::move(load_callback_), 14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->UpdateCallback(true);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_TRUE(load_successful_.value());
  ASSERT_EQ(2u, load_results_->size());
  ASSERT_EQ(2u, db_availabilities_.size());

  ASSERT_TRUE(load_results_->find(kTestFeatureFoo.name) !=
              load_results_->end());
  EXPECT_EQ(14u, (*load_results_)[kTestFeatureFoo.name]);
  ASSERT_TRUE(db_availabilities_.find(kTestFeatureFoo.name) !=
              db_availabilities_.end());
  EXPECT_EQ(14u, db_availabilities_[kTestFeatureFoo.name].day());

  ASSERT_TRUE(load_results_->find(kTestFeatureBar.name) !=
              load_results_->end());
  EXPECT_EQ(14u, (*load_results_)[kTestFeatureBar.name]);
  ASSERT_TRUE(db_availabilities_.find(kTestFeatureBar.name) !=
              db_availabilities_.end());
  EXPECT_EQ(14u, db_availabilities_[kTestFeatureBar.name].day());
}

TEST_F(PersistentAvailabilityStoreTest, TestAllFilterCombinations) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo, kTestFeatureBar},
                                        {kTestFeatureQux, kTestFeatureNop});

  FeatureVector feature_filter;
  feature_filter.push_back(&kTestFeatureFoo);  // Enabled. Not in DB.
  feature_filter.push_back(&kTestFeatureBar);  // Enabled. In DB.
  feature_filter.push_back(&kTestFeatureQux);  // Disabled. Not in DB.
  feature_filter.push_back(&kTestFeatureNop);  // Disabled. In DB.

  db_availabilities_[kTestFeatureBar.name] =
      CreateAvailability(kTestFeatureBar, 10u);
  db_availabilities_[kTestFeatureNop.name] =
      CreateAvailability(kTestFeatureNop, 8u);

  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), feature_filter, std::move(load_callback_), 14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->UpdateCallback(true);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_TRUE(load_successful_.value());
  ASSERT_EQ(2u, load_results_->size());
  ASSERT_EQ(2u, db_availabilities_.size());

  ASSERT_TRUE(load_results_->find(kTestFeatureFoo.name) !=
              load_results_->end());
  EXPECT_EQ(14u, (*load_results_)[kTestFeatureFoo.name]);
  ASSERT_TRUE(db_availabilities_.find(kTestFeatureFoo.name) !=
              db_availabilities_.end());
  EXPECT_EQ(14u, db_availabilities_[kTestFeatureFoo.name].day());

  ASSERT_TRUE(load_results_->find(kTestFeatureBar.name) !=
              load_results_->end());
  EXPECT_EQ(10u, (*load_results_)[kTestFeatureBar.name]);
  ASSERT_TRUE(db_availabilities_.find(kTestFeatureBar.name) !=
              db_availabilities_.end());
  EXPECT_EQ(10u, db_availabilities_[kTestFeatureBar.name].day());
}

TEST_F(PersistentAvailabilityStoreTest, TestAllCombinationsEmptyFilter) {
  scoped_feature_list_.InitWithFeatures({kTestFeatureFoo, kTestFeatureBar},
                                        {kTestFeatureQux, kTestFeatureNop});

  // Empty filter, but the following setup:
  // kTestFeatureFoo: Enabled. Not in DB.
  // kTestFeatureBar: Enabled. In DB.
  // kTestFeatureQux: Disabled. Not in DB.
  // kTestFeatureNop: Disabled. In DB.

  db_availabilities_[kTestFeatureBar.name] =
      CreateAvailability(kTestFeatureBar, 10u);
  db_availabilities_[kTestFeatureNop.name] =
      CreateAvailability(kTestFeatureNop, 8u);

  PersistentAvailabilityStore::LoadAndUpdateStore(
      storage_dir_, CreateDB(), FeatureVector(), std::move(load_callback_),
      14u);

  db_->InitCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->LoadCallback(true);
  EXPECT_FALSE(load_successful_.has_value());

  db_->UpdateCallback(true);

  EXPECT_TRUE(load_successful_.has_value());
  EXPECT_TRUE(load_successful_.value());
  EXPECT_EQ(0u, load_results_->size());
  EXPECT_EQ(0u, db_availabilities_.size());
}

}  // namespace feature_engagement
