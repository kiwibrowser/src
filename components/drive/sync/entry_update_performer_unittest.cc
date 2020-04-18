// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/sync/entry_update_performer.h"

#include <stdint.h>

#include "base/callback_helpers.h"
#include "base/files/file_util.h"
#include "base/md5.h"
#include "base/task_runner_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class EntryUpdatePerformerTest : public file_system::OperationTestBase {
 protected:
  void SetUp() override {
    OperationTestBase::SetUp();
    performer_.reset(new EntryUpdatePerformer(blocking_task_runner(),
                                              delegate(),
                                              scheduler(),
                                              metadata(),
                                              cache(),
                                              loader_controller()));
  }

  // Stores |content| to the cache and mark it as dirty.
  FileError StoreAndMarkDirty(const std::string& local_id,
                              const std::string& content) {
    base::FilePath path;
    if (!base::CreateTemporaryFileInDir(temp_dir(), &path) ||
        !google_apis::test_util::WriteStringToFile(path, content))
      return FILE_ERROR_FAILED;

    // Store the file to cache.
    FileError error = FILE_ERROR_FAILED;
    base::PostTaskAndReplyWithResult(
        blocking_task_runner(),
        FROM_HERE,
        base::Bind(&FileCache::Store,
                   base::Unretained(cache()),
                   local_id, std::string(), path,
                   FileCache::FILE_OPERATION_COPY),
        google_apis::test_util::CreateCopyResultCallback(&error));
    content::RunAllTasksUntilIdle();
    return error;
  }

  std::unique_ptr<EntryUpdatePerformer> performer_;
};

TEST_F(EntryUpdatePerformerTest, UpdateEntry) {
  base::FilePath src_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));
  base::FilePath dest_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/Sub Directory Folder"));

  ResourceEntry src_entry, dest_entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &src_entry));
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(dest_path, &dest_entry));

  // Update local entry.
  base::Time new_last_modified = base::Time::FromInternalValue(
      src_entry.file_info().last_modified()) + base::TimeDelta::FromSeconds(1);
  base::Time new_last_accessed = base::Time::FromInternalValue(
      src_entry.file_info().last_accessed()) + base::TimeDelta::FromSeconds(2);

  src_entry.set_parent_local_id(dest_entry.local_id());
  src_entry.set_title("Moved" + src_entry.title());
  src_entry.mutable_file_info()->set_last_modified(
      new_last_modified.ToInternalValue());
  src_entry.mutable_file_info()->set_last_accessed(
      new_last_accessed.ToInternalValue());
  src_entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry,
                 base::Unretained(metadata()),
                 src_entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Perform server side update.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      src_entry.local_id(),
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify the file is updated on the server.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> gdata_entry;
  fake_service()->GetFileResource(
      src_entry.resource_id(),
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &gdata_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  ASSERT_TRUE(gdata_entry);

  EXPECT_EQ(src_entry.title(), gdata_entry->title());
  EXPECT_EQ(new_last_modified, gdata_entry->modified_date());
  EXPECT_EQ(new_last_accessed, gdata_entry->last_viewed_by_me_date());

  ASSERT_FALSE(gdata_entry->parents().empty());
  EXPECT_EQ(dest_entry.resource_id(), gdata_entry->parents()[0].file_id());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_SetProperties) {
  base::FilePath entry_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(entry_path, &entry));

  Property* const first_property = entry.mutable_new_properties()->Add();
  first_property->set_key("hello");
  first_property->set_value("world");
  first_property->set_visibility(Property_Visibility_PUBLIC);

  Property* const second_property = entry.mutable_new_properties()->Add();
  second_property->set_key("this");
  second_property->set_value("will-change");
  second_property->set_visibility(Property_Visibility_PUBLIC);
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(), FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry, base::Unretained(metadata()),
                 entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Perform server side update.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      entry.local_id(), ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));

  // Add a new property during an update.
  Property* const property = entry.mutable_new_properties()->Add();
  property->set_key("tokyo");
  property->set_value("kyoto");
  property->set_visibility(Property_Visibility_PUBLIC);

  // Modify an existing property during an update.
  second_property->set_value("changed");

  // Change the resource entry during an update.
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(), FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry, base::Unretained(metadata()),
                 entry),
      google_apis::test_util::CreateCopyResultCallback(&error));

  // Wait until the update is fully completed.
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // List of synced properties should be removed from the proto.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(entry_path, &entry));
  ASSERT_EQ(2, entry.new_properties().size());
  EXPECT_EQ("changed", entry.new_properties().Get(0).value());
  EXPECT_EQ("tokyo", entry.new_properties().Get(1).key());
}

// Tests updating metadata of a file with a non-dirty cache file.
TEST_F(EntryUpdatePerformerTest, UpdateEntry_WithNonDirtyCache) {
  base::FilePath src_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  // Download the file content to prepare a non-dirty cache file.
  file_system::DownloadOperation download_operation(
      blocking_task_runner(), delegate(), scheduler(), metadata(), cache(),
      temp_dir());
  FileError error = FILE_ERROR_FAILED;
  base::FilePath cache_file_path;
  std::unique_ptr<ResourceEntry> src_entry;
  download_operation.EnsureFileDownloadedByPath(
      src_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &cache_file_path, &src_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(src_entry);

  // Update the entry locally.
  src_entry->set_title("Updated" + src_entry->title());
  src_entry->set_metadata_edit_state(ResourceEntry::DIRTY);

  error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&ResourceMetadata::RefreshEntry,
                 base::Unretained(metadata()),
                 *src_entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Perform server side update. This shouldn't fail. (crbug.com/358590)
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      src_entry->local_id(),
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Verify the file is updated on the server.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> gdata_entry;
  fake_service()->GetFileResource(
      src_entry->resource_id(),
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &gdata_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  ASSERT_TRUE(gdata_entry);
  EXPECT_EQ(src_entry->title(), gdata_entry->title());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_NotFound) {
  const std::string id = "this ID should result in NOT_FOUND";
  FileError error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      id, ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_NOT_FOUND, error);
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_ContentUpdate) {
  const base::FilePath kFilePath(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  const std::string kResourceId("2_file_resource_id");

  const std::string local_id = GetLocalId(kFilePath);
  EXPECT_FALSE(local_id.empty());

  const std::string kTestFileContent = "I'm being uploaded! Yay!";
  EXPECT_EQ(FILE_ERROR_OK, StoreAndMarkDirty(local_id, kTestFileContent));

  const std::string original_start_page_token =
      fake_service()->start_page_token().start_page_token();

  // The callback will be called upon completion of UpdateEntry().
  FileError error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Check that the server has received an update.
  EXPECT_NE(original_start_page_token,
            fake_service()->start_page_token().start_page_token());

  // Check that the file size is updated to that of the updated content.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  fake_service()->GetFileResource(
      kResourceId,
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &server_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  EXPECT_EQ(static_cast<int64_t>(kTestFileContent.size()),
            server_entry->file_size());

  // Make sure that the cache is no longer dirty.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_ContentUpdateMd5Check) {
  const base::FilePath kFilePath(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  const std::string kResourceId("2_file_resource_id");

  const std::string local_id = GetLocalId(kFilePath);
  EXPECT_FALSE(local_id.empty());

  const std::string kTestFileContent = "I'm being uploaded! Yay!";
  EXPECT_EQ(FILE_ERROR_OK, StoreAndMarkDirty(local_id, kTestFileContent));

  std::string original_start_page_token =
      fake_service()->start_page_token().start_page_token();

  // The callback will be called upon completion of UpdateEntry().
  FileError error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Check that the server has received an update.
  EXPECT_NE(original_start_page_token,
            fake_service()->start_page_token().start_page_token());

  // Check that the file size is updated to that of the updated content.
  google_apis::DriveApiErrorCode gdata_error = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  fake_service()->GetFileResource(
      kResourceId,
      google_apis::test_util::CreateCopyResultCallback(&gdata_error,
                                                       &server_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, gdata_error);
  EXPECT_EQ(static_cast<int64_t>(kTestFileContent.size()),
            server_entry->file_size());

  // Make sure that the cache is no longer dirty.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());

  // Again mark the cache file dirty.
  std::unique_ptr<base::ScopedClosureRunner> file_closer;
  error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&FileCache::OpenForWrite,
                 base::Unretained(cache()),
                 local_id,
                 &file_closer),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  file_closer.reset();

  // And call UpdateEntry again.
  // In this case, although the file is marked as dirty, but the content
  // hasn't been changed. Thus, the actual uploading should be skipped.
  original_start_page_token =
      fake_service()->start_page_token().start_page_token();
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  EXPECT_EQ(original_start_page_token,
            fake_service()->start_page_token().start_page_token());

  // Make sure that the cache is no longer dirty.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_OpenedForWrite) {
  const base::FilePath kFilePath(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  const std::string kResourceId("2_file_resource_id");

  const std::string local_id = GetLocalId(kFilePath);
  EXPECT_FALSE(local_id.empty());

  const std::string kTestFileContent = "I'm being uploaded! Yay!";
  EXPECT_EQ(FILE_ERROR_OK, StoreAndMarkDirty(local_id, kTestFileContent));

  // Emulate a situation where someone is writing to the file.
  std::unique_ptr<base::ScopedClosureRunner> file_closer;
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&FileCache::OpenForWrite,
                 base::Unretained(cache()),
                 local_id,
                 &file_closer),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Update. This should not clear the dirty bit.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Make sure that the cache is still dirty.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_dirty());

  // Close the file.
  file_closer.reset();

  // Update. This should clear the dirty bit.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Make sure that the cache is no longer dirty.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_UploadNewFile) {
  // Create a new file locally.
  const base::FilePath kFilePath(FILE_PATH_LITERAL("drive/root/New File.txt"));

  ResourceEntry parent;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath.DirName(), &parent));

  ResourceEntry entry;
  entry.set_parent_local_id(parent.local_id());
  entry.set_title(kFilePath.BaseName().AsUTF8Unsafe());
  entry.mutable_file_specific_info()->set_content_mime_type("text/plain");
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::AddEntry,
                 base::Unretained(metadata()),
                 entry,
                 &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Update. This should result in creating a new file on the server.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // The entry got a resource ID.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.resource_id().empty());
  EXPECT_EQ(ResourceEntry::CLEAN, entry.metadata_edit_state());

  // Make sure that the cache is no longer dirty.
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());

  // Make sure that we really created a file.
  google_apis::DriveApiErrorCode status = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  fake_service()->GetFileResource(
      entry.resource_id(),
      google_apis::test_util::CreateCopyResultCallback(&status, &server_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, status);
  ASSERT_TRUE(server_entry);
  EXPECT_FALSE(server_entry->IsDirectory());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_NewFileOpendForWrite) {
  // Create a new file locally.
  const base::FilePath kFilePath(FILE_PATH_LITERAL("drive/root/New File.txt"));

  ResourceEntry parent;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath.DirName(), &parent));

  ResourceEntry entry;
  entry.set_parent_local_id(parent.local_id());
  entry.set_title(kFilePath.BaseName().AsUTF8Unsafe());
  entry.mutable_file_specific_info()->set_content_mime_type("text/plain");
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::AddEntry,
                 base::Unretained(metadata()),
                 entry,
                 &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  const std::string kTestFileContent = "This is a new file.";
  EXPECT_EQ(FILE_ERROR_OK, StoreAndMarkDirty(local_id, kTestFileContent));

  // Emulate a situation where someone is writing to the file.
  std::unique_ptr<base::ScopedClosureRunner> file_closer;
  error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&FileCache::OpenForWrite,
                 base::Unretained(cache()),
                 local_id,
                 &file_closer),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Update, but no update is performed because the file is opened.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // The entry hasn't got a resource ID yet.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_TRUE(entry.resource_id().empty());

  // Close the file.
  file_closer.reset();

  // Update. This should result in creating a new file on the server.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // The entry got a resource ID.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kFilePath, &entry));
  EXPECT_FALSE(entry.resource_id().empty());
  EXPECT_EQ(ResourceEntry::CLEAN, entry.metadata_edit_state());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_CreateDirectory) {
  // Create a new directory locally.
  const base::FilePath kPath(FILE_PATH_LITERAL("drive/root/New Directory"));

  ResourceEntry parent;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kPath.DirName(), &parent));

  ResourceEntry entry;
  entry.set_parent_local_id(parent.local_id());
  entry.set_title(kPath.BaseName().AsUTF8Unsafe());
  entry.mutable_file_info()->set_is_directory(true);
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::AddEntry,
                 base::Unretained(metadata()),
                 entry,
                 &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Update. This should result in creating a new directory on the server.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      local_id,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // The entry got a resource ID.
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kPath, &entry));
  EXPECT_FALSE(entry.resource_id().empty());
  EXPECT_EQ(ResourceEntry::CLEAN, entry.metadata_edit_state());

  // Make sure that we really created a directory.
  google_apis::DriveApiErrorCode status = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  fake_service()->GetFileResource(
      entry.resource_id(),
      google_apis::test_util::CreateCopyResultCallback(&status, &server_entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, status);
  ASSERT_TRUE(server_entry);
  EXPECT_TRUE(server_entry->IsDirectory());
}

TEST_F(EntryUpdatePerformerTest, UpdateEntry_InsufficientPermission) {
  base::FilePath src_path(
      FILE_PATH_LITERAL("drive/root/Directory 1/SubDirectory File 1.txt"));

  ResourceEntry src_entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(src_path, &src_entry));

  // Update local entry.
  ResourceEntry updated_entry(src_entry);
  updated_entry.set_title("Moved" + src_entry.title());
  updated_entry.set_metadata_edit_state(ResourceEntry::DIRTY);

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

  // Try to perform update.
  error = FILE_ERROR_FAILED;
  performer_->UpdateEntry(
      src_entry.local_id(),
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // This should result in reverting the local change.
  ResourceEntry result_entry;
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntryById(src_entry.local_id(), &result_entry));
  EXPECT_EQ(src_entry.title(), result_entry.title());
}

}  // namespace internal
}  // namespace drive
