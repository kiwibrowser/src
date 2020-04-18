// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/sync/remove_performer.h"

#include "base/task_runner_util.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"

namespace drive {
namespace internal {

typedef file_system::OperationTestBase RemovePerformerTest;

TEST_F(RemovePerformerTest, RemoveFile) {
  RemovePerformer performer(blocking_task_runner(), delegate(), scheduler(),
                            metadata());

  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  base::FilePath file_in_subdir(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  // Remove a file in root.
  ResourceEntry entry;
  FileError error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &entry));
  performer.Remove(entry.local_id(),
                   ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Remove a file in subdirectory.
  error = FILE_ERROR_FAILED;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_subdir, &entry));
  const std::string resource_id = entry.resource_id();

  performer.Remove(entry.local_id(),
                   ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify the file is indeed removed in the server.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> gdata_entry;
  fake_service()->GetFileResource(
      resource_id,
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &gdata_entry));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  EXPECT_TRUE(gdata_entry->labels().is_trashed());

  // Try removing non-existing file.
  error = FILE_ERROR_FAILED;
  performer.Remove("non-existing-id",
                   ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);
}

TEST_F(RemovePerformerTest, RemoveShared) {
  RemovePerformer performer(blocking_task_runner(), delegate(), scheduler(),
                            metadata());

  const base::FilePath kPathInMyDrive(FILE_PATH_LITERAL(
      "drive/root/shared.txt"));
  const base::FilePath kPathInOther(FILE_PATH_LITERAL(
      "drive/other/shared.txt"));

  // Prepare a shared file to the root folder.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> gdata_entry;
  fake_service()->AddNewFile(
      "text/plain",
      "dummy content",
      fake_service()->GetRootResourceId(),
      kPathInMyDrive.BaseName().AsUTF8Unsafe(),
      true,  // shared_with_me,
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &gdata_entry));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(google_apis::HTTP_CREATED, gdata_error);
  CheckForUpdates();

  // Remove it. Locally, the file should be moved to drive/other.
  ResourceEntry entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kPathInMyDrive, &entry));
  FileError error = FILE_ERROR_FAILED;
  performer.Remove(entry.local_id(),
                   ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntry(kPathInMyDrive, &entry));
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kPathInOther, &entry));

  // Remotely, the entry should have lost its parent.
  gdata_error = google_apis::DRIVE_OTHER_ERROR;
  const std::string resource_id = gdata_entry->file_id();
  fake_service()->GetFileResource(
      resource_id,
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &gdata_entry));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  EXPECT_FALSE(gdata_entry->labels().is_trashed());  // It's not deleted.
  EXPECT_TRUE(gdata_entry->parents().empty());
}

TEST_F(RemovePerformerTest, RemoveLocallyCreatedFile) {
  RemovePerformer performer(blocking_task_runner(), delegate(), scheduler(),
                            metadata());

  // Add an entry without resource ID.
  ResourceEntry entry;
  entry.set_title("New File.txt");
  entry.set_parent_local_id(util::kDriveTrashDirLocalId);

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::AddEntry,
                 base::Unretained(metadata()),
                 entry,
                 &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Remove the entry.
  performer.Remove(local_id, ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntryById(local_id, &entry));
}

TEST_F(RemovePerformerTest, Remove_InsufficientPermission) {
  RemovePerformer performer(blocking_task_runner(), delegate(), scheduler(),
                            metadata());

  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));

  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  // Remove locally.
  ResourceEntry updated_entry(src_entry);
  updated_entry.set_parent_local_id(util::kDriveTrashDirLocalId);

  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry,
                 base::Unretained(metadata()),
                 updated_entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Set user permission to forbid server side update.
  EXPECT_EQ(google_apis::HTTP_SUCCESS, fake_service()->SetUserPermission(
      src_entry.resource_id(), google_apis::drive::PERMISSION_ROLE_READER));

  // Try to perform remove.
  error = FILE_ERROR_FAILED;
  performer.Remove(src_entry.local_id(),
                   ClientContext(USER_INITIATED),
                   google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // This should result in reverting the local change.
  ResourceEntry result_entry;
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntryById(src_entry.local_id(), &result_entry));
  EXPECT_EQ(src_entry.parent_local_id(), result_entry.parent_local_id());

  ASSERT_EQ(1U, delegate()->drive_sync_errors().size());
  EXPECT_EQ(file_system::DRIVE_SYNC_ERROR_DELETE_WITHOUT_PERMISSION,
            delegate()->drive_sync_errors()[0]);
}

}  // namespace internal
}  // namespace drive
