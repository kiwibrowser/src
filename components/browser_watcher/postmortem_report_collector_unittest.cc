// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/postmortem_report_collector.h"

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/debug/activity_analyzer.h"
#include "base/debug/activity_tracker.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "base/process/process_handle.h"
#include "base/stl_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/threading/platform_thread.h"
#include "components/browser_watcher/stability_data_names.h"
#include "components/browser_watcher/stability_report_extractor.h"
#include "components/metrics/system_session_analyzer_win.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"

namespace browser_watcher {

using base::debug::ActivityData;
using base::debug::ActivityTrackerMemoryAllocator;
using base::debug::ActivityUserData;
using base::debug::GlobalActivityTracker;
using base::debug::ThreadActivityTracker;
using base::File;
using base::FilePath;
using base::FilePersistentMemoryAllocator;
using base::MemoryMappedFile;
using base::PersistentMemoryAllocator;
using base::WrapUnique;
using crashpad::CrashReportDatabase;
using crashpad::Settings;
using crashpad::UUID;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

namespace {

const char kProductName[] = "TestProduct";
const char kVersionNumber[] = "TestVersionNumber";
const char kChannelName[] = "TestChannel";

class MockPostmortemReportCollector final : public PostmortemReportCollector {
 public:
  explicit MockPostmortemReportCollector(CrashReportDatabase* crash_database)
      : PostmortemReportCollector(kProductName,
                                  kVersionNumber,
                                  kChannelName,
                                  crash_database,
                                  nullptr) {}

  MOCK_METHOD2(CollectOneReport,
               CollectionStatus(const FilePath&, StabilityReport* report));
  MOCK_METHOD4(WriteReportToMinidump,
               bool(StabilityReport* report,
                    const crashpad::UUID& client_id,
                    const crashpad::UUID& report_id,
                    crashpad::FileWriterInterface* minidump_file));
};

class MockSystemSessionAnalyzer : public metrics::SystemSessionAnalyzer {
 public:
  MockSystemSessionAnalyzer() : SystemSessionAnalyzer(10U) {}
  MOCK_METHOD1(IsSessionUnclean, Status(base::Time timestamp));
};

// Checks if two proto messages are the same based on their serializations. Note
// this only works if serialization is deterministic, which is not guaranteed.
// In practice, serialization is deterministic (even for protocol buffers with
// maps) and such matchers are common in the Chromium code base. Also note that
// in the context of this test, false positive matches are the problem and these
// are not possible (otherwise serialization would be ambiguous). False
// negatives would lead to test failure and developer action. Alternatives are:
// 1) a generic matcher (likely not possible without reflections, missing from
// lite runtime),  2) a specialized matcher or 3) implementing deterministic
// serialization.
// TODO(manzagop): switch a matcher with guarantees.
MATCHER_P(EqualsProto, message, "") {
  std::string expected_serialized;
  std::string actual_serialized;
  message.SerializeToString(&expected_serialized);
  arg.SerializeToString(&actual_serialized);
  return expected_serialized == actual_serialized;
}

}  // namespace

class PostmortemReportCollectorProcessTest
    : public ::testing::TestWithParam<bool> {
 public:
  void SetUpTest(bool system_session_clean,
                 bool expect_write_dump,
                 bool provide_crash_db) {
    // Create a dummy debug file.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    debug_file_ = temp_dir_.GetPath().AppendASCII("foo-1.pma");
    {
      base::ScopedFILE file(base::OpenFile(debug_file_, "w"));
      ASSERT_NE(file.get(), nullptr);
    }
    ASSERT_TRUE(base::PathExists(debug_file_));

    if (provide_crash_db) {
      database_ = CrashReportDatabase::Initialize(
          temp_dir_.GetPath().AppendASCII("db"));
    }

    collector_.reset(new MockPostmortemReportCollector(database_.get()));

    // Expect a single collection call.
    StabilityReport report;
    report.mutable_system_state()->set_session_state(
        system_session_clean ? SystemState::CLEAN : SystemState::UNCLEAN);
    EXPECT_CALL(*collector_, CollectOneReport(debug_file_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(report), Return(SUCCESS)));

    if (expect_write_dump) {
      EXPECT_CALL(*collector_, WriteReportToMinidump(_, _, _, _))
          .Times(1)
          .WillOnce(Return(true));
    }
  }
  void ValidateHistograms(int unclean_cnt, int unclean_system_cnt) {
    histogram_tester_.ExpectBucketCount("ActivityTracker.Collect.Status",
                                        UNCLEAN_SHUTDOWN, unclean_cnt);
    histogram_tester_.ExpectBucketCount("ActivityTracker.Collect.Status",
                                        UNCLEAN_SESSION, unclean_system_cnt);
  }
  void CollectReports(bool is_session_clean, bool provide_crash_db) {
    SetUpTest(is_session_clean, provide_crash_db, provide_crash_db);

    // Run the test.
    std::vector<FilePath> debug_files{debug_file_};
    collector_->Process(debug_files);
    ASSERT_FALSE(base::PathExists(debug_file_));

    if (provide_crash_db) {
      std::vector<CrashReportDatabase::Report> reports;
      ASSERT_EQ(CrashReportDatabase::kNoError,
                database_->GetPendingReports(&reports));
      EXPECT_EQ(1u, reports.size());
    }
  }

 protected:
  base::HistogramTester histogram_tester_;
  base::ScopedTempDir temp_dir_;
  FilePath debug_file_;
  std::unique_ptr<CrashReportDatabase> database_;
  std::unique_ptr<MockPostmortemReportCollector> collector_;
};

TEST_P(PostmortemReportCollectorProcessTest, ProcessCleanSession) {
  bool provide_crash_db = GetParam();
  CollectReports(true, provide_crash_db);
  int expected_unclean = 1;
  int expected_system_unclean = 0;
  ValidateHistograms(expected_unclean, expected_system_unclean);
}

TEST_P(PostmortemReportCollectorProcessTest, ProcessUncleanSession) {
  bool provide_crash_db = GetParam();
  CollectReports(false, provide_crash_db);
  int expected_unclean = 1;
  int expected_system_unclean = 1;
  ValidateHistograms(expected_unclean, expected_system_unclean);
}

TEST_P(PostmortemReportCollectorProcessTest, ProcessStuckFile) {
  bool provide_crash_db = GetParam();
  bool system_session_clean = true;
  bool expect_write_dump = false;
  SetUpTest(system_session_clean, expect_write_dump, provide_crash_db);

  // Open the stability debug file to prevent its deletion.
  base::ScopedFILE file(base::OpenFile(debug_file_, "w"));
  ASSERT_NE(file.get(), nullptr);

  // Run the test.
  std::vector<FilePath> debug_files{debug_file_};
  collector_->Process(debug_files);
  ASSERT_TRUE(base::PathExists(debug_file_));

  histogram_tester_.ExpectBucketCount("ActivityTracker.Collect.Status",
                                      DEBUG_FILE_DELETION_FAILED, 1);
  int expected_unclean = 0;
  int expected_system_unclean = 0;
  ValidateHistograms(expected_unclean, expected_system_unclean);
}

INSTANTIATE_TEST_CASE_P(WithCrashDatabase,
                        PostmortemReportCollectorProcessTest,
                        ::testing::Values(true));
INSTANTIATE_TEST_CASE_P(WithoutCrashDatabase,
                        PostmortemReportCollectorProcessTest,
                        ::testing::Values(true));

TEST(PostmortemReportCollectorTest, CollectEmptyFile) {
  // Create an empty file.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.GetPath().AppendASCII("empty.pma");
  {
    base::ScopedFILE file(base::OpenFile(file_path, "w"));
    ASSERT_NE(file.get(), nullptr);
  }
  ASSERT_TRUE(PathExists(file_path));

  // Validate collection: an empty file cannot suppport an analyzer.
  std::unique_ptr<CrashReportDatabase> crash_db(
      CrashReportDatabase::Initialize(temp_dir.GetPath().AppendASCII("db")));
  PostmortemReportCollector collector(kProductName, kVersionNumber,
                                      kChannelName, crash_db.get(), nullptr);
  StabilityReport report;
  ASSERT_EQ(ANALYZER_CREATION_FAILED,
            collector.CollectOneReport(file_path, &report));
}

TEST(PostmortemReportCollectorTest, CollectRandomFile) {
  // Create a file with content we don't expect to be valid for a debug file.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.GetPath().AppendASCII("invalid_content.pma");
  {
    base::ScopedFILE file(base::OpenFile(file_path, "w"));
    ASSERT_NE(file.get(), nullptr);
    // Assuming this size is greater than the minimum size of a debug file.
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
      data[i] = i % UINT8_MAX;
    ASSERT_EQ(data.size(),
              fwrite(&data.at(0), sizeof(uint8_t), data.size(), file.get()));
  }
  ASSERT_TRUE(PathExists(file_path));

  // Validate collection: random content appears as though there is not
  // stability data.
  std::unique_ptr<CrashReportDatabase> crash_db(
      CrashReportDatabase::Initialize(temp_dir.GetPath().AppendASCII("db")));
  PostmortemReportCollector collector(kProductName, kVersionNumber,
                                      kChannelName, crash_db.get(), nullptr);
  StabilityReport report;
  ASSERT_NE(SUCCESS, collector.CollectOneReport(file_path, &report));
}

class PostmortemReportCollectorCollectionFromGlobalTrackerTest
    : public testing::Test {
 public:
  const int kMemorySize = 1 << 20;  // 1MiB

  PostmortemReportCollectorCollectionFromGlobalTrackerTest() {}
  ~PostmortemReportCollectorCollectionFromGlobalTrackerTest() override {
    GlobalActivityTracker* global_tracker = GlobalActivityTracker::Get();
    if (global_tracker) {
      global_tracker->ReleaseTrackerForCurrentThreadForTesting();
      delete global_tracker;
    }
  }

  void SetUp() override {
    testing::Test::SetUp();

    // Set up a debug file path.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    debug_file_path_ = temp_dir_.GetPath().AppendASCII("debug.pma");
  }

  const FilePath& debug_file_path() { return debug_file_path_; }

 protected:
  base::ScopedTempDir temp_dir_;
  FilePath debug_file_path_;
};

TEST_F(PostmortemReportCollectorCollectionFromGlobalTrackerTest,
       SystemStateTest) {
  // Setup.
  GlobalActivityTracker::CreateWithFile(debug_file_path(), kMemorySize, 0ULL,
                                        "", 3);
  ActivityUserData& process_data = GlobalActivityTracker::Get()->process_data();
  process_data.SetInt(kStabilityStartTimestamp, 12345LL);

  // Collect.
  MockSystemSessionAnalyzer analyzer;
  EXPECT_CALL(analyzer,
              IsSessionUnclean(base::Time::FromInternalValue(12345LL)))
      .Times(1)
      .WillOnce(Return(metrics::SystemSessionAnalyzer::CLEAN));
  std::unique_ptr<CrashReportDatabase> crash_db(
      CrashReportDatabase::Initialize(temp_dir_.GetPath().AppendASCII("db")));
  PostmortemReportCollector collector(kProductName, kVersionNumber,
                                      kChannelName, crash_db.get(), &analyzer);
  StabilityReport report;
  ASSERT_EQ(SUCCESS, collector.CollectOneReport(debug_file_path(), &report));

  // Validate the report.
  ASSERT_EQ(SystemState::CLEAN, report.system_state().session_state());
}

}  // namespace browser_watcher
