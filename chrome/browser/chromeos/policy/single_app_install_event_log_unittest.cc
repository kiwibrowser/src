// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/single_app_install_event_log.h"

#include <stdint.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace policy {

namespace {

static const int kLogCapacity = 1024;

static const char kPackageName[] = "com.example.app";
static const int64_t kTimestamp = 12345;
static const char kFileName[] = "event.log";

}  // namespace

class SingleAppInstallEventLogTest : public testing::Test {
 protected:
  SingleAppInstallEventLogTest() {}

  // testing::Test:
  void SetUp() override {
    log_.reset(new SingleAppInstallEventLog(kPackageName));
  }

  void VerifyHeader(bool incomplete) {
    EXPECT_TRUE(report_.has_package());
    EXPECT_EQ(kPackageName, report_.package());
    EXPECT_TRUE(report_.has_incomplete());
    EXPECT_EQ(incomplete, report_.incomplete());
  }

  void CreateFile() {
    temp_dir_.reset(new base::ScopedTempDir);
    ASSERT_TRUE(temp_dir_->CreateUniqueTempDir());
    file_.reset(new base::File(temp_dir_->GetPath().Append(kFileName),
                               base::File::FLAG_CREATE_ALWAYS |
                                   base::File::FLAG_WRITE |
                                   base::File::FLAG_READ));
  }

  std::unique_ptr<SingleAppInstallEventLog> log_;
  em::AppInstallReport report_;
  std::unique_ptr<base::ScopedTempDir> temp_dir_;
  std::unique_ptr<base::File> file_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleAppInstallEventLogTest);
};

// Verify that the package name is returned correctly.
TEST_F(SingleAppInstallEventLogTest, GetPackage) {
  EXPECT_EQ(kPackageName, log_->package());
}

// Do not add any log entries. Serialize the log. Verify that the serialization
// contains the the correct header data (package name, incomplete flag) and no
// log entries.
TEST_F(SingleAppInstallEventLogTest, SerializeEmpty) {
  EXPECT_TRUE(log_->empty());
  EXPECT_EQ(0, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  EXPECT_EQ(0, report_.log_size());
}

// Add a log entry. Verify that the entry is serialized correctly.
TEST_F(SingleAppInstallEventLogTest, AddAndSerialize) {
  em::AppInstallReportLogEvent event;
  event.set_timestamp(kTimestamp);
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  log_->Add(event);
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(1, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(1, report_.log_size());
  std::string original_event;
  event.SerializeToString(&original_event);
  std::string log_event;
  report_.log(0).SerializeToString(&log_event);
  EXPECT_EQ(original_event, log_event);
}

// Add 10 log entries. Verify that they are serialized correctly. Then, clear
// the serialized log entries and verify that the log becomes empty.
TEST_F(SingleAppInstallEventLogTest, SerializeAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(10, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(10, report_.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, report_.log(i).timestamp());
  }

  log_->ClearSerialized();
  EXPECT_TRUE(log_->empty());
  EXPECT_EQ(0, log_->size());

  report_.Clear();
  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  EXPECT_EQ(0, report_.log_size());
}

// Add 10 log entries. Serialize the log. Add 10 more log entries. Clear the
// serialized log entries. Verify that the log now contains the last 10 entries.
TEST_F(SingleAppInstallEventLogTest, SerializeAddAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(10, log_->size());

  log_->Serialize(&report_);

  for (int i = 10; i < 20; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(20, log_->size());

  log_->ClearSerialized();
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(10, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(10, report_.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i + 10, report_.log(i).timestamp());
  }
}

// Add more entries than the log has capacity for. Serialize the log. Verify
// that the serialization contains the most recent log entries and the
// incomplete flag is set. Then, clear the serialized log entries. Verify that
// the log becomes empty and the incomplete flag is unset.
TEST_F(SingleAppInstallEventLogTest, OverflowSerializeAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(true /* incomplete */);
  ASSERT_EQ(kLogCapacity, report_.log_size());
  for (int i = 0; i < kLogCapacity; ++i) {
    EXPECT_EQ(i + 1, report_.log(i).timestamp());
  }

  log_->ClearSerialized();
  EXPECT_TRUE(log_->empty());
  EXPECT_EQ(0, log_->size());

  report_.Clear();
  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  EXPECT_EQ(0, report_.log_size());
}

// Add more entries than the log has capacity for. Serialize the log. Add one
// more log entry. Clear the serialized log entries. Verify that the log now
// contains the most recent entry and the incomplete flag is unset.
TEST_F(SingleAppInstallEventLogTest, OverflowSerializeAddAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->Serialize(&report_);

  event.set_timestamp(kLogCapacity + 1);
  log_->Add(event);
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->ClearSerialized();
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(1, log_->size());

  report_.Clear();
  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(1, report_.log_size());
  EXPECT_EQ(kLogCapacity + 1, report_.log(0).timestamp());
}

// Add more entries than the log has capacity for. Serialize the log. Add
// exactly as many entries as the log has capacity for. Clear the serialized log
// entries. Verify that the log now contains the most recent entries and the
// incomplete flag is unset.
TEST_F(SingleAppInstallEventLogTest, OverflowSerializeFillAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->Serialize(&report_);

  for (int i = 0; i < kLogCapacity; ++i) {
    event.set_timestamp(kLogCapacity + i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->ClearSerialized();
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  report_.Clear();
  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(kLogCapacity, report_.log_size());
  for (int i = 0; i < kLogCapacity; ++i) {
    EXPECT_EQ(i + kLogCapacity, report_.log(i).timestamp());
  }
}

// Add more entries than the log has capacity for. Serialize the log. Add more
// entries than the log has capacity for. Clear the serialized log entries.
// Verify that the log now contains the most recent entries and the incomplete
// flag is set.
TEST_F(SingleAppInstallEventLogTest, OverflowSerializeOverflowAndClear) {
  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->Serialize(&report_);

  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(kLogCapacity + i);
    log_->Add(event);
  }
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  log_->ClearSerialized();
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(kLogCapacity, log_->size());

  report_.Clear();
  log_->Serialize(&report_);
  VerifyHeader(true /* incomplete */);
  ASSERT_EQ(kLogCapacity, report_.log_size());
  for (int i = 0; i < kLogCapacity; ++i) {
    EXPECT_EQ(i + kLogCapacity + 1, report_.log(i).timestamp());
  }
}

// Load log from a file that is not open. Verify that the operation fails.
TEST_F(SingleAppInstallEventLogTest, FailLoad) {
  base::File invalid_file;
  std::unique_ptr<SingleAppInstallEventLog> log =
      std::make_unique<SingleAppInstallEventLog>(kPackageName);
  EXPECT_FALSE(SingleAppInstallEventLog::Load(&invalid_file, &log));
  EXPECT_FALSE(log);
}

// Add a log entry. Store the log to a file that is not open. Verify that the
// operation fails and the log is not modified.
TEST_F(SingleAppInstallEventLogTest, FailStore) {
  em::AppInstallReportLogEvent event;
  event.set_timestamp(0);
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  log_->Add(event);
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(1, log_->size());

  base::File invalid_file;
  EXPECT_FALSE(log_->Store(&invalid_file));
  EXPECT_EQ(kPackageName, log_->package());
  EXPECT_FALSE(log_->empty());
  EXPECT_EQ(1, log_->size());

  log_->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(1, report_.log_size());
  EXPECT_EQ(0, report_.log(0).timestamp());
}

// Store an empty log. Load the log. Verify that that the log contents are
// loaded correctly.
TEST_F(SingleAppInstallEventLogTest, StoreEmptyAndLoad) {
  ASSERT_NO_FATAL_FAILURE(CreateFile());

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);

  std::unique_ptr<SingleAppInstallEventLog> log;
  EXPECT_TRUE(SingleAppInstallEventLog::Load(file_.get(), &log));
  ASSERT_TRUE(log);
  EXPECT_EQ(kPackageName, log->package());
  EXPECT_TRUE(log->empty());
  EXPECT_EQ(0, log->size());

  log->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(0, report_.log_size());
}

// Populate and store a log. Load the log. Verify that that the log contents are
// loaded correctly.
TEST_F(SingleAppInstallEventLogTest, StoreAndLoad) {
  ASSERT_NO_FATAL_FAILURE(CreateFile());

  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);

  std::unique_ptr<SingleAppInstallEventLog> log;
  EXPECT_TRUE(SingleAppInstallEventLog::Load(file_.get(), &log));
  ASSERT_TRUE(log);
  EXPECT_EQ(kPackageName, log->package());
  EXPECT_FALSE(log->empty());
  EXPECT_EQ(10, log->size());

  log->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(10, report_.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, report_.log(i).timestamp());
  }
}

// Add more entries than the log has capacity for. Store the log. Load the log.
// Verify that the log is marked as incomplete.
TEST_F(SingleAppInstallEventLogTest, OverflowStoreAndLoad) {
  ASSERT_NO_FATAL_FAILURE(CreateFile());

  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < kLogCapacity + 1; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);

  std::unique_ptr<SingleAppInstallEventLog> log;
  EXPECT_TRUE(SingleAppInstallEventLog::Load(file_.get(), &log));
  ASSERT_TRUE(log);
  EXPECT_EQ(kPackageName, log->package());
  EXPECT_FALSE(log->empty());
  EXPECT_EQ(kLogCapacity, log->size());

  log->Serialize(&report_);
  VerifyHeader(true /* incomplete */);
}

// Populate and serialize a log. Store the log. Load the log. Clear serialized
// entries in the loaded log. Verify that no entries are removed.
TEST_F(SingleAppInstallEventLogTest, SerializeStoreLoadAndClear) {
  ASSERT_NO_FATAL_FAILURE(CreateFile());

  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }

  log_->Serialize(&report_);

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);

  std::unique_ptr<SingleAppInstallEventLog> log;
  EXPECT_TRUE(SingleAppInstallEventLog::Load(file_.get(), &log));
  ASSERT_TRUE(log);
  EXPECT_EQ(kPackageName, log->package());
  EXPECT_FALSE(log->empty());
  EXPECT_EQ(10, log->size());

  log->ClearSerialized();
  EXPECT_FALSE(log->empty());
  EXPECT_EQ(10, log->size());

  report_.Clear();
  log->Serialize(&report_);
  VerifyHeader(false /* incomplete */);
  ASSERT_EQ(10, report_.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, report_.log(i).timestamp());
  }
}

// Add 20 log entries. Store the log. Truncate the file to the length of a log
// containing 10 log entries plus one byte. Load the log. Verify that the log
// contains the first 10 log entries and is marked as incomplete.
TEST_F(SingleAppInstallEventLogTest, LoadTruncated) {
  ASSERT_NO_FATAL_FAILURE(CreateFile());

  em::AppInstallReportLogEvent event;
  event.set_event_type(em::AppInstallReportLogEvent::SUCCESS);
  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i);
    log_->Add(event);
  }

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);
  const ssize_t size = file_->GetLength();

  for (int i = 0; i < 10; ++i) {
    event.set_timestamp(i + 10);
    log_->Add(event);
  }

  log_->Store(file_.get());
  file_->Seek(base::File::FROM_BEGIN, 0);
  file_->SetLength(size + 1);

  std::unique_ptr<SingleAppInstallEventLog> log;
  EXPECT_FALSE(SingleAppInstallEventLog::Load(file_.get(), &log));
  ASSERT_TRUE(log);
  EXPECT_EQ(kPackageName, log->package());
  EXPECT_FALSE(log->empty());
  EXPECT_EQ(10, log->size());

  report_.Clear();
  log->Serialize(&report_);
  VerifyHeader(true /* incomplete */);
  ASSERT_EQ(10, report_.log_size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, report_.log(i).timestamp());
  }
}

}  // namespace policy
