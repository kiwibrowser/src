// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/move_operation.h"

#include "components/drive/file_change.h"
#include "components/drive/file_system/operation_test_base.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

class MoveOperationTest : public OperationTestBase {
 protected:
  void SetUp() override {
   OperationTestBase::SetUp();
   operation_.reset(new MoveOperation(blocking_task_runner(),
                                      delegate(),
                                      metadata()));
  }

  std::unique_ptr<MoveOperation> operation_;
};

TEST_F(MoveOperationTest, MoveFileInSameDirectory) {
  const base::FilePath src_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));
  const base::FilePath dest_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/Test.log"));

  ResourceEntry src_entry, dest_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &src_entry));
  ASSERT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(dest_path, &dest_entry));

  FileError error = FILE_ERROR_FAILED;
  operation_->Move(src_path,
                   dest_path,
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(dest_path, &dest_entry));
  EXPECT_EQ(src_entry.local_id(), dest_entry.local_id());
  EXPECT_EQ(ResourceEntry::DIRTY, dest_entry.metadata_edit_state());
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(src_path, &src_entry));

  EXPECT_EQ(2U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(src_path));
  EXPECT_TRUE(delegate()->get_changed_files().count(dest_path));

  EXPECT_EQ(1U, delegate()->updated_local_ids().size());
  EXPECT_TRUE(delegate()->updated_local_ids().count(src_entry.local_id()));
}

TEST_F(MoveOperationTest, MoveFileFromRootToSubDirectory) {
  base::FilePath src_path(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  base::FilePath dest_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/Test.log"));

  ResourceEntry src_entry, dest_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &src_entry));
  ASSERT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(dest_path, &dest_entry));

  FileError error = FILE_ERROR_FAILED;
  operation_->Move(src_path,
                   dest_path,
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(dest_path, &dest_entry));
  EXPECT_EQ(src_entry.local_id(), dest_entry.local_id());
  EXPECT_EQ(ResourceEntry::DIRTY, dest_entry.metadata_edit_state());
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(src_path, &src_entry));

  EXPECT_EQ(2U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(src_path));
  EXPECT_TRUE(delegate()->get_changed_files().count(dest_path));

  EXPECT_EQ(1U, delegate()->updated_local_ids().size());
  EXPECT_TRUE(delegate()->updated_local_ids().count(src_entry.local_id()));
}

TEST_F(MoveOperationTest, MoveNotExistingFile) {
  base::FilePath src_path(FILE_PATH_LITERAL("drive/root/Dummy file.txt"));
  base::FilePath dest_path(FILE_PATH_LITERAL("drive/root/Test.log"));

  FileError error = FILE_ERROR_OK;
  operation_->Move(src_path,
                   dest_path,
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(src_path, &entry));
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(dest_path, &entry));
}

TEST_F(MoveOperationTest, MoveFileToNonExistingDirectory) {
  base::FilePath src_path(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  base::FilePath dest_path(FILE_PATH_LITERAL("drive/root/Dummy/Test.log"));

  FileError error = FILE_ERROR_OK;
  operation_->Move(src_path,
                   dest_path,
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &entry));
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(dest_path, &entry));
}

// Test the case where the parent of |dest_file_path| is a existing file,
// not a directory.
TEST_F(MoveOperationTest, MoveFileToInvalidPath) {
  base::FilePath src_path(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  base::FilePath dest_path(
      FILE_PATH_LITERAL("drive/root/Duplicate Name.txt/Test.log"));

  FileError error = FILE_ERROR_OK;
  operation_->Move(src_path,
                   dest_path,
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_A_DIRECTORY, error);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &entry));
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(dest_path, &entry));
}

}  // namespace file_system
}  // namespace drive
