// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/app_install_event_log.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <sstream>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace policy {

namespace {

static const int kLogCapacity = 1024;
static const int kMaxLogs = 1024;

static const char kFirstPackageName[] = "com.example.first";
static const char kSecondPackageName[] = "com.example.second";
static const char kPackageNameTemplate[] = "com.example.";
static const char kFileName[] = "event.log";

}  // namespace

class AppInstallEventLogTest : public testing::Test {
 protected:
  AppInstallEventLogTest() {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_name_ = temp_dir_.GetPath().Append(kFileName);
    log_ = std::make_unique<AppInstallEventLog>(file_name_);
  }

  void VerifyTenLogEntriesEach(int first_app_timestamp_offset,
                               int second_app_timestamp_offset) {
    ASSERT_EQ(2, report_.app_install_report_size());
    const int first_app_index =
        report_.app_install_report(0).package() == kFirstPackageName ? 0 : 1;
    const int second_app_index = 1 - first_app_index;

    const em::AppInstallReport& first_app_log =
        report_.app_install_report(first_app_index);
    EXPECT_EQ(kFirstPackageName, first_app_log.package());
    ASSERT_EQ(10, first_app_log.log_size());
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(i + first_app_timestamp_offset,
                first_app_log.log(i).timestamp());
    }

    const em::AppInstallReport& second_app_log =
        report_.app_install_report(second_app_index);
    EXPECT_EQ(kSecondPackageName, second_app_log.package());
    ASSERT_EQ(10, second_app_log.log_size());
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(i + second_app_timestamp_offset,
                second_app_log.log(i).timestamp());
    }
  }

  void OverflowMaxLogs() {
    em::AppInstallReportLogEvent event;
    event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
    for (int i = 0; i < kMaxLogs - 1; ++i) {
      event.set_timestamp(i);
      std::stringstream package;
      package << kPackageNameTemplate << i;
      log_->Add(package.str(), event);
    }
    for (int i = 0; i < 10; ++i) {
      event.set_timestamp(i + kMaxLogs - 1);
      log_->Add(kFirstPackageName, event);
    }
    for (int i = 0; i < 20; ++i) {
      event.set_timestamp(i + kMaxLogs + 29);
      log_->Add(kSecondPackageName, event);
    }
  }

  void VerifyOneLogEntryEachPlusFirstApp(int first_app_log_entries) {
    ASSERT_EQ(kMaxLogs, report_.app_install_report_size());
    std::map<std::string, em::AppInstallReport> logs;
    for (int i = 0; i < kMaxLogs; ++i) {
      logs[report_.app_install_report(i).package()] =
          report_.app_install_report(i);
    }

    for (int i = 0; i < kMaxLogs - 1; ++i) {
      std::stringstream package;
      package << kPackageNameTemplate << i;
      const auto log = logs.find(package.str());
      ASSERT_NE(logs.end(), log);
      EXPECT_EQ(package.str(), log->second.package());
      ASSERT_EQ(1, log->second.log_size());
      EXPECT_EQ(i, log->second.log(0).timestamp());
      EXPECT_EQ(em::AppInstallReportLogEvent::SUCCESS,
                log->second.log(0).event_type());
    }

    const auto log = logs.find(kFirstPackageName);
    ASSERT_NE(logs.end(), log);
    EXPECT_EQ(kFirstPackageName, log->second.package());
    ASSERT_EQ(first_app_log_entries, log->second.log_size());
    for (int i = 0; i < first_app_log_entries; ++i) {
      EXPECT_EQ(i + kMaxLogs - 1, log->second.log(i).timestamp());
      EXPECT_EQ(em::AppInstallReportLogEvent::SUCCESS,
                log->second.log(i).event_type());
    }

    EXPECT_EQ(logs.end(), logs.find(kSecondPackageName));
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath file_name_;
  std::unique_ptr<AppInstallEventLog> log_;
  em::AppInstallReportRequest report_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppInstallEventLogTest);
};

// Do not add any log entries. Serialize the log. Verify that the serialization
// contains no log entries.
TEST_F(AppInstallEventLogTest, SerializeEmpty) {
  EXPECT_EQ(0, log_->total_size());
  EXPECT_EQ(0, log_->max_size());

  log_->Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Populate the logs for two apps. Verify that the entries are serialized
// correctly.
TEST_F(AppInstallEventLogTest, AddAndSerialize) {
  em::AppInstallReportLogEvent event;
  event.set_timestamp(0);
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  log_->Add(kFirstPackageName, event);
  event.set_timestamp(1);
  log_->Add(kSecondPackageName, event);
  event.set_timestamp(2);
  log_->Add(kFirstPackageName, event);
  EXPECT_EQ(3, log_->total_size());
  EXPECT_EQ(2, log_->max_size());

  log_->Serialize(&report_);
  ASSERT_EQ(2, report_.app_install_report_size());
  const int first_app_index =
      report_.app_install_report(0).package() == kFirstPackageName ? 0 : 1;
  const int second_app_index = 1 - first_app_index;

  const em::AppInstallReport& first_app_log =
      report_.app_install_report(first_app_index);
  EXPECT_EQ(kFirstPackageName, first_app_log.package());
  ASSERT_EQ(2, first_app_log.log_size());
  EXPECT_EQ(0, first_app_log.log(0).timestamp());
  EXPECT_EQ(em::AppInstallReportLogEvent::SUCCESS,
            first_app_log.log(0).event_type());
  EXPECT_EQ(2, first_app_log.log(1).timestamp());
  EXPECT_EQ(em::AppInstallReportLogEvent::SUCCESS,
            first_app_log.log(1).event_type());

  const em::AppInstallReport& second_app_log =
      report_.app_install_report(second_app_index);
  EXPECT_EQ(kSecondPackageName, second_app_log.package());
  ASSERT_EQ(1, second_app_log.log_size());
  EXPECT_EQ(1, second_app_log.log(0).timestamp());
  EXPECT_EQ(em::AppInstallReportLogEvent::SUCCESS,
            second_app_log.log(0).event_type());
}

// Add 10 log entries for an app. Serialize the log. Clear the serialized log
// entries and verify that the log becomes empty. Then, serialize the log again
// and verify it contains no log entries.
TEST_F(AppInstallEventLogTest, SerializeAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }
  EXPECT_EQ(10, log_->total_size());
  EXPECT_EQ(10, log_->max_size());

  log_->Serialize(&report_);

  log_->ClearSerialized();
  EXPECT_EQ(0, log_->total_size());
  EXPECT_EQ(0, log_->max_size());

  report_.Clear();
  log_->Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Add 10 log entries for a first app. Serialize the log. Add 10 more log
// entries each for the first and a second app. Clear the serialized log
// entries. Then, serialize the log again. Verify that it now contains the
// entries added after the first serialization.
TEST_F(AppInstallEventLogTest, SerializeAddClearAndSerialize) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }
  EXPECT_EQ(10, log_->total_size());
  EXPECT_EQ(10, log_->max_size());

  log_->Serialize(&report_);

  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i + 10);
    log_->Add(kFirstPackageName, event);
  }
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i + 20);
    log_->Add(kSecondPackageName, event);
  }
  EXPECT_EQ(30, log_->total_size());
  EXPECT_EQ(20, log_->max_size());

  log_->ClearSerialized();
  EXPECT_EQ(20, log_->total_size());
  EXPECT_EQ(10, log_->max_size());

  log_->Serialize(&report_);
  VerifyTenLogEntriesEach(10 /* first_app_timestamp_offset */,
                          20 /* second_app_timestamp_offset*/);
}

// Add entries for as many apps as the log has capacity for. Add entries for one
// more app. Serialize the log. Verify that the log entries for the last app
// were ignored. Then, clear the serialized log entries. Verify that the log
// becomes empty.
TEST_F(AppInstallEventLogTest, OverflowSerializeAndClear) {
  OverflowMaxLogs();
  EXPECT_EQ(kMaxLogs + 9, log_->total_size());
  EXPECT_EQ(10, log_->max_size());

  log_->Serialize(&report_);
  VerifyOneLogEntryEachPlusFirstApp(10 /* first_app_log_entries */);

  log_->ClearSerialized();
  EXPECT_EQ(0, log_->total_size());
  EXPECT_EQ(0, log_->max_size());

  report_.Clear();
  log_->Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Add entries for as many apps as the log has capacity for. Add entries for one
// more app. Add more entries for one of the apps already in the log. Serialize
// the log. Verify that the log entries for the last app were ignored. Then,
// clear the serialized log entries. Verify that the log becomes empty.
TEST_F(AppInstallEventLogTest, OverflowAddSerializeAndClear) {
  OverflowMaxLogs();
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 20; ++i) {
    event.set_timestamp(i + kMaxLogs + 9);
    log_->Add(kFirstPackageName, event);
  }
  EXPECT_EQ(kMaxLogs + 29, log_->total_size());
  EXPECT_EQ(30, log_->max_size());

  log_->Serialize(&report_);
  VerifyOneLogEntryEachPlusFirstApp(30 /* first_app_log_entries */);

  log_->ClearSerialized();
  EXPECT_EQ(0, log_->total_size());
  EXPECT_EQ(0, log_->max_size());

  report_.Clear();
  log_->Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Add entries for as many apps as the log has capacity for. Add entries for one
// more app. Serialize the log. Add more entries for one of the apps already in
// the log and another app. Clear the log. Verify that the log now contains the
// entries added after serialization for the app that was already in the log.
TEST_F(AppInstallEventLogTest, OverflowSerializeAddAndClear) {
  OverflowMaxLogs();
  EXPECT_EQ(kMaxLogs + 9, log_->total_size());
  EXPECT_EQ(10, log_->max_size());

  log_->Serialize(&report_);

  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 20; ++i) {
    event.set_timestamp(i + kMaxLogs + 9);
    log_->Add(kFirstPackageName, event);
  }
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i + kMaxLogs + 49);
    log_->Add(kSecondPackageName, event);
  }

  log_->ClearSerialized();
  EXPECT_EQ(20, log_->total_size());
  EXPECT_EQ(20, log_->max_size());

  report_.Clear();
  log_->Serialize(&report_);
  ASSERT_EQ(1, report_.app_install_report_size());
  const em::AppInstallReport& app_log = report_.app_install_report(0);
  EXPECT_EQ(kFirstPackageName, app_log.package());
  ASSERT_EQ(20, app_log.log_size());
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(i + kMaxLogs + 9, app_log.log(i).timestamp());
  }
}

// Add 10 log entries for a first app and more entries than the log has capacity
// for for a second app. Verify that the total and maximum log sizes are
// reported correctly.
TEST_F(AppInstallEventLogTest, OverflowSingleApp) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i + 10);
    log_->Add(kSecondPackageName, event);
  }
  EXPECT_EQ(10 + kLogCapacity, log_->total_size());
  EXPECT_EQ(kLogCapacity, log_->max_size());
}

// Create an empty log. Store the log. Verify that no log file is created.
TEST_F(AppInstallEventLogTest, Store) {
  log_->Store();
  EXPECT_FALSE(base::PathExists(file_name_));
}

// Add a log entry. Store the log. Verify that a log file is created. Then,
// delete the log file. Store the log. Verify that no log file is created.
TEST_F(AppInstallEventLogTest, AddStoreAndStore) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  event.set_timestamp(0);
  log_->Add(kFirstPackageName, event);
  log_->Store();
  EXPECT_TRUE(base::PathExists(file_name_));

  EXPECT_TRUE(base::DeleteFile(file_name_, false /* recursive */));

  log_->Store();
  EXPECT_FALSE(base::PathExists(file_name_));
}

// Serialize the log. Clear the serialized log entries. Store the log. Verify
// that no log file is created.
TEST_F(AppInstallEventLogTest, SerializeClearAndStore) {
  log_->Serialize(&report_);
  log_->ClearSerialized();
  log_->Store();
  EXPECT_FALSE(base::PathExists(file_name_));
}

// Add a log entry. Serialize the log. Clear the serialized log entries. Store
// the log. Verify that a log file is created. Verify that that the log contents
// are loaded correctly.
TEST_F(AppInstallEventLogTest, AddSerializeCleaStoreAndLoad) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  event.set_timestamp(0);
  log_->Add(kFirstPackageName, event);
  log_->Serialize(&report_);
  log_->ClearSerialized();
  log_->Store();
  EXPECT_TRUE(base::PathExists(file_name_));

  AppInstallEventLog log(file_name_);
  EXPECT_EQ(0, log.total_size());
  EXPECT_EQ(0, log.max_size());

  report_.Clear();
  log.Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Populate and store a log. Load the log. Verify that that the log contents are
// loaded correctly. Then, delete the log file. Store the log. Verify that no
// log file is created.
TEST_F(AppInstallEventLogTest, StoreLoadAndStore) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i + 10);
    log_->Add(kSecondPackageName, event);
  }

  log_->Store();

  AppInstallEventLog log(file_name_);
  EXPECT_EQ(20, log.total_size());
  EXPECT_EQ(10, log.max_size());

  log.Serialize(&report_);
  VerifyTenLogEntriesEach(0 /* first_app_timestamp_offset */,
                          10 /* second_app_timestamp_offset*/);

  EXPECT_TRUE(base::DeleteFile(file_name_, false /* recursive */));

  log.Store();
  EXPECT_FALSE(base::PathExists(file_name_));
}

// Populate and serialize a log. Store the log. Load the log. Clear serialized
// entries in the loaded log. Verify that no entries are removed.
TEST_F(AppInstallEventLogTest, SerializeStoreLoadAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }

  log_->Serialize(&report_);

  log_->Store();

  AppInstallEventLog log(file_name_);
  EXPECT_EQ(10, log.total_size());
  EXPECT_EQ(10, log.max_size());

  log.ClearSerialized();
  EXPECT_EQ(10, log.total_size());
  EXPECT_EQ(10, log.max_size());

  report_.Clear();
  log.Serialize(&report_);
  ASSERT_EQ(1, report_.app_install_report_size());
  const em::AppInstallReport& app_log = report_.app_install_report(0);
  EXPECT_EQ(kFirstPackageName, app_log.package());
  ASSERT_EQ(10, app_log.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, app_log.log(i).timestamp());
  }
}

// Populate and serialize a log. Store the log. Change the version in the log
// file. Load the log. Verify that the log contents are not loaded.
TEST_F(AppInstallEventLogTest, LoadVersionMismatch) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }

  log_->Store();

  std::unique_ptr<base::File> file = std::make_unique<base::File>(
      file_name_,
      base::File::FLAG_OPEN | base::File::FLAG_READ | base::File::FLAG_WRITE);
  int64_t version;
  EXPECT_EQ(static_cast<ssize_t>(sizeof(version)),
            file->Read(0, reinterpret_cast<char*>(&version), sizeof(version)));
  --version;
  EXPECT_EQ(
      static_cast<ssize_t>(sizeof(version)),
      file->Write(0, reinterpret_cast<const char*>(&version), sizeof(version)));
  file.reset();

  AppInstallEventLog log(file_name_);
  EXPECT_EQ(0, log.total_size());
  EXPECT_EQ(0, log.max_size());

  log.Serialize(&report_);
  EXPECT_EQ(0, report_.app_install_report_size());
}

// Add 10 log entries each for two apps. Store the log. Truncate the file to the
// length of a log containing 10 log entries for one app plus one byte. Load the
// log. Verify that the log contains 10 logs for one app.
TEST_F(AppInstallEventLogTest, LoadTruncated) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kFirstPackageName, event);
  }

  log_->Store();

  std::unique_ptr<base::File> file = std::make_unique<base::File>(
      file_name_, base::File::FLAG_OPEN | base::File::FLAG_READ);
  const ssize_t size = file->GetLength();
  file.reset();

  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(kSecondPackageName, event);
  }

  file = std::make_unique<base::File>(
      file_name_, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  file->SetLength(size + 1);
  file.reset();

  AppInstallEventLog log(file_name_);
  EXPECT_EQ(10, log.total_size());
  EXPECT_EQ(10, log.max_size());

  log.Serialize(&report_);
  ASSERT_EQ(1, report_.app_install_report_size());
  const std::string& package_name = report_.app_install_report(0).package();
  ASSERT_TRUE(package_name == kFirstPackageName ||
              package_name == kSecondPackageName);

  const em::AppInstallReport& app_log = report_.app_install_report(0);
  EXPECT_EQ(package_name, app_log.package());
  ASSERT_EQ(10, app_log.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, app_log.log(i).timestamp());
  }
}

}  // namespace policy
