// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/open_file_operation.h"

#include <stdint.h>

#include <map>
#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task_runner_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_change.h"
#include "components/drive/file_errors.h"
#include "components/drive/file_system/operation_test_base.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

class OpenFileOperationTest : public OperationTestBase {
 protected:
  void SetUp() override {
    OperationTestBase::SetUp();

    operation_.reset(new OpenFileOperation(
        blocking_task_runner(), delegate(), scheduler(), metadata(), cache(),
        temp_dir()));
  }

  std::unique_ptr<OpenFileOperation> operation_;
};

TEST_F(OpenFileOperationTest, OpenExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      OPEN_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  int64_t local_file_size;
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(file_size, local_file_size);

  ASSERT_FALSE(close_callback.is_null());
  close_callback.Run();
  EXPECT_EQ(1U, delegate()->updated_local_ids().count(src_entry.local_id()));
}

TEST_F(OpenFileOperationTest, OpenNonExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/not-exist.txt"));

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      OPEN_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);
  EXPECT_TRUE(close_callback.is_null());
}

TEST_F(OpenFileOperationTest, CreateExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      CREATE_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_EXISTS, error);
  EXPECT_TRUE(close_callback.is_null());
}

TEST_F(OpenFileOperationTest, CreateNonExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/not-exist.txt"));

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      CREATE_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(file_in_root));

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  int64_t local_file_size;
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(0, local_file_size);  // Should be an empty file.

  ASSERT_FALSE(close_callback.is_null());
  close_callback.Run();
  EXPECT_EQ(1U,
            delegate()->updated_local_ids().count(GetLocalId(file_in_root)));
}

TEST_F(OpenFileOperationTest, OpenOrCreateExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      OPEN_OR_CREATE_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  // Notified because 'available offline' status of the existing file changes.
  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(file_in_root));

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  int64_t local_file_size;
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(file_size, local_file_size);

  ASSERT_FALSE(close_callback.is_null());
  close_callback.Run();
  EXPECT_EQ(1U, delegate()->updated_local_ids().count(src_entry.local_id()));

  ResourceEntry result_entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &result_entry));
  EXPECT_TRUE(result_entry.file_specific_info().cache_state().is_present());
  EXPECT_TRUE(result_entry.file_specific_info().cache_state().is_dirty());
}

TEST_F(OpenFileOperationTest, OpenOrCreateNonExistingFile) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/not-exist.txt"));

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      OPEN_OR_CREATE_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  int64_t local_file_size;
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(0, local_file_size);  // Should be an empty file.

  ASSERT_FALSE(close_callback.is_null());
  close_callback.Run();
  EXPECT_EQ(1U,
            delegate()->updated_local_ids().count(GetLocalId(file_in_root)));
}

TEST_F(OpenFileOperationTest, OpenFileTwice) {
  const base::FilePath file_in_root(
      FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  base::Closure close_callback;
  operation_->OpenFile(
      file_in_root,
      OPEN_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  int64_t local_file_size;
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(file_size, local_file_size);

  // Open again.
  error = FILE_ERROR_FAILED;
  base::Closure close_callback2;
  operation_->OpenFile(
      file_in_root,
      OPEN_FILE,
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &close_callback2));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(base::PathExists(file_path));
  ASSERT_TRUE(base::GetFileSize(file_path, &local_file_size));
  EXPECT_EQ(file_size, local_file_size);

  ASSERT_FALSE(close_callback.is_null());
  ASSERT_FALSE(close_callback2.is_null());

  close_callback.Run();

  // There still remains a client opening the file, so it shouldn't be
  // uploaded yet.
  EXPECT_TRUE(delegate()->updated_local_ids().empty());

  close_callback2.Run();

  // Here, all the clients close the file, so it should be uploaded then.
  EXPECT_EQ(1U, delegate()->updated_local_ids().count(src_entry.local_id()));
}

}  // namespace file_system
}  // namespace drive
