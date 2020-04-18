// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/remove_operation.h"

#include "components/drive/file_change.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

typedef OperationTestBase RemoveOperationTest;

TEST_F(RemoveOperationTest, RemoveFile) {
  RemoveOperation operation(blocking_task_runner(), delegate(), metadata(),
                            cache());

  base::FilePath nonexisting_file(
      FILE_PATH_LITERAL("drive/root/Dummy file.txt"));
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  base::FilePath file_in_subdir(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  // Remove a file in root.
  ResourceEntry entry;
  FileError error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &entry));
  operation.Remove(file_in_root,
                   false,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntry(file_in_root, &entry));

  const std::string id_file_in_root = entry.local_id();
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntryById(id_file_in_root, &entry));
  EXPECT_EQ(util::kDriveTrashDirLocalId, entry.parent_local_id());

  // Remove a file in subdirectory.
  error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_subdir, &entry));
  const std::string resource_id = entry.resource_id();

  operation.Remove(file_in_subdir,
                   false,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(file_in_subdir, &entry));

  const std::string id_file_in_subdir = entry.local_id();
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntryById(id_file_in_subdir, &entry));
  EXPECT_EQ(util::kDriveTrashDirLocalId, entry.parent_local_id());

  // Try removing non-existing file.
  error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(nonexisting_file, &entry));
  operation.Remove(nonexisting_file,
                   false,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);

  // Verify delegate notifications.
  EXPECT_EQ(2U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(file_in_root));
  EXPECT_TRUE(delegate()->get_changed_files().count(file_in_subdir));

  EXPECT_EQ(2U, delegate()->updated_local_ids().size());
  EXPECT_TRUE(delegate()->updated_local_ids().count(id_file_in_root));
  EXPECT_TRUE(delegate()->updated_local_ids().count(id_file_in_subdir));
}

TEST_F(RemoveOperationTest, RemoveDirectory) {
  RemoveOperation operation(blocking_task_runner(), delegate(), metadata(),
                            cache());

  base::FilePath empty_dir(FILE_PATH_LITERAL(
      "drive/root/Directory 1/Sub Directory Folder/Sub Sub Directory Folder"));
  base::FilePath non_empty_dir(FILE_PATH_LITERAL(
      "drive/root/Directory 1"));
  base::FilePath file_in_non_empty_dir(FILE_PATH_LITERAL(
      "drive/root/Directory 1/SubDirectory File 1.txt"));

  // Empty directory can be removed even with is_recursive = false.
  FileError error = FILE_ERROR_FAILED;
  ResourceEntry entry;
  operation.Remove(empty_dir,
                   false,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(empty_dir, &entry));

  // Non-empty directory, cannot.
  error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(non_empty_dir, &entry));
  operation.Remove(non_empty_dir,
                   false,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_EMPTY, error);
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntry(non_empty_dir, &entry));

  // With is_recursive = true, it can be deleted, however. Descendant entries
  // are removed together.
  error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntry(file_in_non_empty_dir, &entry));
  operation.Remove(non_empty_dir,
                   true,  // is_recursive
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(non_empty_dir, &entry));
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(file_in_non_empty_dir, &entry));
}

}  // namespace file_system
}  // namespace drive
