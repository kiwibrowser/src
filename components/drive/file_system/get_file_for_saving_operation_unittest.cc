// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/get_file_for_saving_operation.h"

#include <stdint.h>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_change.h"
#include "components/drive/file_errors.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/file_write_watcher.h"
#include "components/drive/job_scheduler.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

namespace {

// If OnCacheFileUploadNeededByOperation is called, records the local ID and
// calls |quit_closure|.
class TestDelegate : public OperationDelegate {
 public:
  void set_quit_closure(const base::Closure& quit_closure) {
    quit_closure_ = quit_closure;
  }

  const std::string& updated_local_id() const {
    return updated_local_id_;
  }

  // OperationDelegate overrides.
  void OnEntryUpdatedByOperation(const ClientContext& /* context */,
                                 const std::string& local_id) override {
    updated_local_id_ = local_id;
    if (!quit_closure_.is_null())
      quit_closure_.Run();
  }

 private:
  std::string updated_local_id_;
  base::Closure quit_closure_;
};

}  // namespace

class GetFileForSavingOperationTest : public OperationTestBase {
 protected:
  // FileWriteWatcher requires TYPE_IO message loop to run.
  GetFileForSavingOperationTest()
      : OperationTestBase(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  void SetUp() override {
    OperationTestBase::SetUp();

    operation_.reset(new GetFileForSavingOperation(
        logger(), blocking_task_runner(), &delegate_, scheduler(), metadata(),
        cache(), temp_dir()));
    operation_->file_write_watcher_for_testing()->DisableDelayForTesting();
  }

  TestDelegate delegate_;
  std::unique_ptr<GetFileForSavingOperation> operation_;
};

TEST_F(GetFileForSavingOperationTest, GetFileForSaving_Exist) {
  base::FilePath drive_path(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(drive_path, &src_entry));

  // Run the operation.
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  base::FilePath local_path;
  operation_->GetFileForSaving(
      drive_path,
      google_apis::test_util::CreateCopyResultCallback(
          &error, &local_path, &entry));
  content::RunAllTasksUntilIdle();

  // Checks that the file is retrieved.
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(src_entry.resource_id(), entry->resource_id());

  // Checks that it presents in cache and marked dirty.
  EXPECT_TRUE(entry->file_specific_info().cache_state().is_present());
  EXPECT_TRUE(entry->file_specific_info().cache_state().is_dirty());

  // Write something to the cache and checks that the event is reported.
  {
    base::RunLoop run_loop;
    delegate_.set_quit_closure(run_loop.QuitClosure());
    google_apis::test_util::WriteStringToFile(local_path, "hello");
    run_loop.Run();
    EXPECT_EQ(GetLocalId(drive_path), delegate_.updated_local_id());
  }
}

TEST_F(GetFileForSavingOperationTest, GetFileForSaving_NotExist) {
  base::FilePath drive_path(FILE_PATH_LITERAL("drive/root/NotExist.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(drive_path, &src_entry));

  // Run the operation.
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  base::FilePath local_path;
  operation_->GetFileForSaving(
      drive_path,
      google_apis::test_util::CreateCopyResultCallback(
          &error, &local_path, &entry));
  content::RunAllTasksUntilIdle();

  // Checks that the file is created and retrieved.
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(drive_path, &src_entry));
  int64_t size = -1;
  EXPECT_TRUE(base::GetFileSize(local_path, &size));
  EXPECT_EQ(0, size);
}

TEST_F(GetFileForSavingOperationTest, GetFileForSaving_Directory) {
  base::FilePath drive_path(FILE_PATH_LITERAL("drive/root/Directory 1"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(drive_path, &src_entry));
  ASSERT_TRUE(src_entry.file_info().is_directory());

  // Run the operation.
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  base::FilePath local_path;
  operation_->GetFileForSaving(
      drive_path,
      google_apis::test_util::CreateCopyResultCallback(
          &error, &local_path, &entry));
  content::RunAllTasksUntilIdle();

  // Checks that an error is returned.
  EXPECT_EQ(FILE_ERROR_EXISTS, error);
}

}  // namespace file_system
}  // namespace drive
