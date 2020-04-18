// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/truncate_operation.h"

#include <stdint.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task_runner_util.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_system/operation_test_base.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

class TruncateOperationTest : public OperationTestBase {
 protected:
  void SetUp() override {
    OperationTestBase::SetUp();

    operation_.reset(new TruncateOperation(
        blocking_task_runner(), delegate(), scheduler(),
        metadata(), cache(), temp_dir()));
  }

  std::unique_ptr<TruncateOperation> operation_;
};

TEST_F(TruncateOperationTest, Truncate) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  // Make sure the file has at least 2 bytes.
  ASSERT_GE(file_size, 2);

  FileError error = FILE_ERROR_FAILED;
  operation_->Truncate(
      file_in_root,
      1,  // Truncate to 1 byte.
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  base::FilePath local_path;
  error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::FileCache::GetFile,
                 base::Unretained(cache()),
                 GetLocalId(file_in_root), &local_path),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(FILE_ERROR_OK, error);

  // The local file should be truncated.
  int64_t local_file_size = 0;
  base::GetFileSize(local_path, &local_file_size);
  EXPECT_EQ(1, local_file_size);
}

TEST_F(TruncateOperationTest, NegativeSize) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  // Make sure the file has at least 2 bytes.
  ASSERT_GE(file_size, 2);

  FileError error = FILE_ERROR_FAILED;
  operation_->Truncate(
      file_in_root,
      -1,  // Truncate to "-1" byte.
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_INVALID_OPERATION, error);
}

TEST_F(TruncateOperationTest, HostedDocument) {
  base::FilePath file_in_root(FILE_PATH_LITERAL(
      "drive/root/Document 1 excludeDir-test.gdoc"));

  FileError error = FILE_ERROR_FAILED;
  operation_->Truncate(
      file_in_root,
      1,  // Truncate to 1 byte.
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_INVALID_OPERATION, error);
}

TEST_F(TruncateOperationTest, Extend) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  FileError error = FILE_ERROR_FAILED;
  operation_->Truncate(
      file_in_root,
      file_size + 10,  // Extend to 10 bytes.
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  base::FilePath local_path;
  error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::FileCache::GetFile,
                 base::Unretained(cache()),
                 GetLocalId(file_in_root), &local_path),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(FILE_ERROR_OK, error);

  // The local file should be truncated.
  std::string content;
  ASSERT_TRUE(base::ReadFileToString(local_path, &content));

  EXPECT_EQ(file_size + 10, static_cast<int64_t>(content.size()));
  // All trailing 10 bytes should be '\0'.
  EXPECT_EQ(std::string(10, '\0'), content.substr(file_size));
}

}  // namespace file_system
}  // namespace drive
