// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_opt_out_store_sql.h"

#include <map>
#include <memory>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/previews/core/previews_black_list_item.h"
#include "components/previews/core/previews_opt_out_store.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace previews {

namespace {

const base::FilePath::CharType kOptOutFilename[] = FILE_PATH_LITERAL("OptOut");

}  // namespace

class PreviewsOptOutStoreSQLTest : public testing::Test {
 public:
  PreviewsOptOutStoreSQLTest()
      : field_trials_(new base::FieldTrialList(nullptr)) {}
  ~PreviewsOptOutStoreSQLTest() override {}

  // Called when |store_| is done loading.
  void OnLoaded(std::unique_ptr<BlackListItemMap> black_list_map,
                std::unique_ptr<PreviewsBlackListItem> host_indifferent_item) {
    black_list_map_ = std::move(black_list_map);
    host_indifferent_item_ = std::move(host_indifferent_item);
  }

  // Initializes the store and get the data from it.
  void Load() {
    store_->LoadBlackList(base::Bind(&PreviewsOptOutStoreSQLTest::OnLoaded,
                                     base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  // Destroys the database connection and |store_|.
  void DestroyStore() {
    store_.reset();
    base::RunLoop().RunUntilIdle();
  }

  // Creates a store that operates on one thread.
  void Create(std::unique_ptr<PreviewsTypeList> enabled_previews) {
    store_ = std::make_unique<PreviewsOptOutStoreSQL>(
        base::ThreadTaskRunnerHandle::Get(),
        base::ThreadTaskRunnerHandle::Get(),
        temp_dir_.GetPath().Append(kOptOutFilename),
        std::move(enabled_previews));
  }

  // Sets up initialization of |store_|.
  void CreateAndLoad(std::unique_ptr<PreviewsTypeList> enabled_previews) {
    Create(std::move(enabled_previews));
    Load();
  }

  // Creates a directory for the test.
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

  // Delete |store_| if it hasn't been deleted.
  void TearDown() override { DestroyStore(); }

 protected:
  void ResetFieldTrials() {
    // Destroy existing FieldTrialList before creating new one to avoid DCHECK.
    field_trials_.reset();
    field_trials_.reset(new base::FieldTrialList(nullptr));
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
  }

  base::HistogramTester histogram_tester_;

  base::MessageLoop message_loop_;

  // The backing SQL store.
  std::unique_ptr<PreviewsOptOutStoreSQL> store_;

  // The map returned from |store_|.
  std::unique_ptr<BlackListItemMap> black_list_map_;

  // The host indifferent item from |store_|
  std::unique_ptr<PreviewsBlackListItem> host_indifferent_item_;

  // The directory for the database.
  base::ScopedTempDir temp_dir_;

 private:
  std::unique_ptr<base::FieldTrialList> field_trials_;
};

TEST_F(PreviewsOptOutStoreSQLTest, TestErrorRecovery) {
  // Creates the database and corrupt to test the recovery method.
  std::string test_host = "host.com";
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  store_->AddPreviewNavigation(true, test_host, PreviewsType::OFFLINE,
                               base::Time::Now());
  base::RunLoop().RunUntilIdle();
  DestroyStore();

  // Corrupts the database by adjusting the header size.
  EXPECT_TRUE(sql::test::CorruptSizeInHeader(
      temp_dir_.GetPath().Append(kOptOutFilename)));
  base::RunLoop().RunUntilIdle();

  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  // The data should be recovered.
  EXPECT_EQ(1U, black_list_map_->size());
  auto iter = black_list_map_->find(test_host);

  EXPECT_NE(black_list_map_->end(), iter);
  EXPECT_EQ(1U, iter->second->OptOutRecordsSizeForTesting());
}

TEST_F(PreviewsOptOutStoreSQLTest, TestPersistance) {
  // Tests if data is stored as expected in the SQLite database.
  std::string test_host = "host.com";
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectUniqueSample("Previews.OptOut.DBRowCount", 0, 1);
  base::Time now = base::Time::Now();
  store_->AddPreviewNavigation(true, test_host, PreviewsType::OFFLINE, now);
  base::RunLoop().RunUntilIdle();

  // Replace the store effectively destroying the current one and forcing it
  // to write its data to disk.
  DestroyStore();

  // Reload and test for persistence
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  EXPECT_EQ(1U, black_list_map_->size());
  auto iter = black_list_map_->find(test_host);

  EXPECT_NE(black_list_map_->end(), iter);
  EXPECT_EQ(1U, iter->second->OptOutRecordsSizeForTesting());
  EXPECT_EQ(now, iter->second->most_recent_opt_out_time().value());
  EXPECT_EQ(1U, host_indifferent_item_->OptOutRecordsSizeForTesting());
  EXPECT_EQ(now, host_indifferent_item_->most_recent_opt_out_time().value());
  histogram_tester_.ExpectBucketCount("Previews.OptOut.DBRowCount", 1, 1);
  histogram_tester_.ExpectTotalCount("Previews.OptOut.DBRowCount", 2);
}

TEST_F(PreviewsOptOutStoreSQLTest, TestMaxRows) {
  // Tests that the number of rows are culled down to the row limit at each
  // load.
  std::string test_host_a = "host_a.com";
  std::string test_host_b = "host_b.com";
  std::string test_host_c = "host_c.com";
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  size_t row_limit = 2;
  std::string row_limit_string = base::NumberToString(row_limit);
  command_line->AppendSwitchASCII("previews-max-opt-out-rows",
                                  row_limit_string);
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectUniqueSample("Previews.OptOut.DBRowCount", 0, 1);
  base::SimpleTestClock clock;

  // Create three different entries with different hosts.
  store_->AddPreviewNavigation(true, test_host_a, PreviewsType::OFFLINE,
                               clock.Now());
  clock.Advance(base::TimeDelta::FromSeconds(1));

  store_->AddPreviewNavigation(true, test_host_b, PreviewsType::OFFLINE,
                               clock.Now());
  base::Time host_b_time = clock.Now();
  clock.Advance(base::TimeDelta::FromSeconds(1));

  store_->AddPreviewNavigation(false, test_host_c, PreviewsType::OFFLINE,
                               clock.Now());
  base::RunLoop().RunUntilIdle();
  // Replace the store effectively destroying the current one and forcing it
  // to write its data to disk.
  DestroyStore();

  // Reload and test for persistence
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectBucketCount("Previews.OptOut.DBRowCount",
                                      static_cast<int>(row_limit) + 1, 1);
  // The delete happens after the load, so it is possible to load more than
  // |row_limit| into the in memory map.
  EXPECT_EQ(row_limit + 1, black_list_map_->size());
  EXPECT_EQ(row_limit + 1,
            host_indifferent_item_->OptOutRecordsSizeForTesting());

  DestroyStore();
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectBucketCount("Previews.OptOut.DBRowCount",
                                      static_cast<int>(row_limit), 1);

  EXPECT_EQ(row_limit, black_list_map_->size());
  auto iter_host_b = black_list_map_->find(test_host_b);
  auto iter_host_c = black_list_map_->find(test_host_c);

  EXPECT_EQ(black_list_map_->end(), black_list_map_->find(test_host_a));
  EXPECT_NE(black_list_map_->end(), iter_host_b);
  EXPECT_NE(black_list_map_->end(), iter_host_c);
  EXPECT_EQ(host_b_time,
            iter_host_b->second->most_recent_opt_out_time().value());
  EXPECT_EQ(1U, iter_host_b->second->OptOutRecordsSizeForTesting());
  EXPECT_EQ(host_b_time,
            host_indifferent_item_->most_recent_opt_out_time().value());
  EXPECT_EQ(row_limit, host_indifferent_item_->OptOutRecordsSizeForTesting());
  histogram_tester_.ExpectTotalCount("Previews.OptOut.DBRowCount", 3);
}

TEST_F(PreviewsOptOutStoreSQLTest, TestMaxRowsPerHost) {
  // Tests that each host is limited to |row_limit| rows.
  std::string test_host = "host.com";
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  size_t row_limit = 2;
  std::string row_limit_string = base::NumberToString(row_limit);
  command_line->AppendSwitchASCII("previews-max-opt-out-rows-per-host",
                                  row_limit_string);
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectUniqueSample("Previews.OptOut.DBRowCount", 0, 1);
  base::SimpleTestClock clock;

  base::Time last_opt_out_time;
  for (size_t i = 0; i < row_limit; i++) {
    store_->AddPreviewNavigation(true, test_host, PreviewsType::OFFLINE,
                                 clock.Now());
    last_opt_out_time = clock.Now();
    clock.Advance(base::TimeDelta::FromSeconds(1));
  }

  clock.Advance(base::TimeDelta::FromSeconds(1));
  store_->AddPreviewNavigation(false, test_host, PreviewsType::OFFLINE,
                               clock.Now());

  base::RunLoop().RunUntilIdle();
  // Replace the store effectively destroying the current one and forcing it
  // to write its data to disk.
  DestroyStore();

  // Reload and test for persistence.
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectBucketCount("Previews.OptOut.DBRowCount",
                                      static_cast<int>(row_limit), 1);

  EXPECT_EQ(1U, black_list_map_->size());
  auto iter = black_list_map_->find(test_host);

  EXPECT_NE(black_list_map_->end(), iter);
  EXPECT_EQ(last_opt_out_time,
            iter->second->most_recent_opt_out_time().value());
  EXPECT_EQ(row_limit, iter->second->OptOutRecordsSizeForTesting());
  EXPECT_EQ(row_limit, host_indifferent_item_->OptOutRecordsSizeForTesting());
  clock.Advance(base::TimeDelta::FromSeconds(1));
  // If both entries' opt out states are stored correctly, then this should not
  // be black listed.
  EXPECT_FALSE(iter->second->IsBlackListed(clock.Now()));
  histogram_tester_.ExpectTotalCount("Previews.OptOut.DBRowCount", 2);
}

TEST_F(PreviewsOptOutStoreSQLTest, TestPreviewsDisabledClearsBlacklistEntry) {
  // Tests if data is cleared for previews type when it is disabled.
  // Enable offline previews and add black list entry for it.
  std::map<std::string, std::string> params;
  std::string test_host = "host.com";
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectUniqueSample("Previews.OptOut.DBRowCount", 0, 1);
  base::Time now = base::Time::Now();
  store_->AddPreviewNavigation(true, test_host, PreviewsType::OFFLINE, now);
  base::RunLoop().RunUntilIdle();

  // Force data write to database then reload it and verify black list entry
  // is present.
  DestroyStore();
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 0});
  CreateAndLoad(std::move(enabled_previews));
  auto iter = black_list_map_->find(test_host);
  EXPECT_NE(black_list_map_->end(), iter);
  EXPECT_EQ(1U, iter->second->OptOutRecordsSizeForTesting());

  DestroyStore();
  enabled_previews.reset(new PreviewsTypeList);
  CreateAndLoad(std::move(enabled_previews));
  iter = black_list_map_->find(test_host);
  EXPECT_EQ(black_list_map_->end(), iter);

}

TEST_F(PreviewsOptOutStoreSQLTest,
       TestPreviewsVersionUpdateClearsBlacklistEntry) {
  // Tests if data is cleared for new version of previews type.
  // Enable offline previews and add black list entry for it.
  std::string test_host = "host.com";
  std::unique_ptr<PreviewsTypeList> enabled_previews(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 1});
  CreateAndLoad(std::move(enabled_previews));
  histogram_tester_.ExpectUniqueSample("Previews.OptOut.DBRowCount", 0, 1);
  base::Time now = base::Time::Now();
  store_->AddPreviewNavigation(true, test_host, PreviewsType::OFFLINE, now);
  base::RunLoop().RunUntilIdle();

  // Force data write to database then reload it and verify black list entry
  // is present.
  DestroyStore();
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 1});
  CreateAndLoad(std::move(enabled_previews));
  auto iter = black_list_map_->find(test_host);
  EXPECT_NE(black_list_map_->end(), iter);
  EXPECT_EQ(1U, iter->second->OptOutRecordsSizeForTesting());

  DestroyStore();
  enabled_previews.reset(new PreviewsTypeList);
  enabled_previews->push_back({PreviewsType::OFFLINE, 2});
  CreateAndLoad(std::move(enabled_previews));
  iter = black_list_map_->find(test_host);
  EXPECT_EQ(black_list_map_->end(), iter);
}

}  // namespace net
