// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/sync/entry_revert_performer.h"

#include "base/task_runner_util.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class EntryRevertPerformerTest : public file_system::OperationTestBase {
 protected:
  void SetUp() override {
   OperationTestBase::SetUp();
   performer_.reset(new EntryRevertPerformer(blocking_task_runner(),
                                             delegate(),
                                             scheduler(),
                                             metadata()));
  }

  std::unique_ptr<EntryRevertPerformer> performer_;
};

TEST_F(EntryRevertPerformerTest, RevertEntry) {
  base::FilePath path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));
  base::FilePath updated_path(FILE_PATH_LITERAL(
      "drive/root/Directory 1/UpdatedSubDirectory File 1.txt"));

  ResourceEntry src_entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(path, &src_entry));

  // Update local entry.
  ResourceEntry updated_entry = src_entry;
  updated_entry.set_title("Updated" + src_entry.title());
  updated_entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry,
                 base::Unretained(metadata()), updated_entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Revert local change.
  error = FILE_ERROR_FAILED;
  performer_->RevertEntry(
      src_entry.local_id(),
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify local change is reverted.
  ResourceEntry result_entry;
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntryById(src_entry.local_id(), &result_entry));
  EXPECT_EQ(src_entry.title(), result_entry.title());
  EXPECT_EQ(ResourceEntry::CLEAN, result_entry.metadata_edit_state());

  EXPECT_EQ(2U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(path));
  EXPECT_TRUE(delegate()->get_changed_files().count(updated_path));
}

TEST_F(EntryRevertPerformerTest, RevertEntry_NotFoundOnServer) {
  ResourceEntry my_drive;
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntry(util::GetDriveMyDriveRootPath(), &my_drive));

  // Add a new entry with a nonexistent resource ID.
  ResourceEntry entry;
  entry.set_title("New File.txt");
  entry.set_resource_id("non-existent-resource-id");
  entry.set_parent_local_id(my_drive.local_id());

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::AddEntry,
                 base::Unretained(metadata()), entry, &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Revert local change. The added entry should be removed.
  error = FILE_ERROR_FAILED;
  performer_->RevertEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify the entry was deleted locally.
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, GetLocalResourceEntryById(local_id, &entry));

  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().CountDirectory(
      util::GetDriveMyDriveRootPath()));
}

TEST_F(EntryRevertPerformerTest, RevertEntry_TrashedOnServer) {
  base::FilePath path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(path, &entry));

  // Trash the entry on the server.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  fake_service()->TrashResource(
      entry.resource_id(),
      google_apis::test_util::CreateCopyResultCallback(&gdata_error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, gdata_error);

  // Revert local change. The entry should be removed.
  FileError error = FILE_ERROR_FAILED;
  performer_->RevertEntry(
      entry.local_id(),
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify the entry was deleted locally.
  EXPECT_EQ(FILE_ERROR_NOT_FOUND,
            GetLocalResourceEntryById(entry.local_id(), &entry));

  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(path));
}

}  // namespace internal
}  // namespace drive
