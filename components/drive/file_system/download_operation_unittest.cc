// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/download_operation.h"

#include <stddef.h>
#include <stdint.h>

#include "base/files/file_util.h"
#include "base/task_runner_util.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

class DownloadOperationTest : public OperationTestBase {
 protected:
  void SetUp() override {
    OperationTestBase::SetUp();

    operation_.reset(new DownloadOperation(
        blocking_task_runner(), delegate(), scheduler(), metadata(), cache(),
        temp_dir()));
  }

  std::unique_ptr<DownloadOperation> operation_;
};

TEST_F(DownloadOperationTest,
       EnsureFileDownloadedByPath_FromServer_EnoughSpace) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  // Pretend we have enough space.
  fake_free_disk_space_getter()->set_default_value(
      file_size + drive::internal::kMinFreeSpaceInBytes);

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->file_specific_info().is_hosted_document());

  // The transfered file is cached and the change of "offline available"
  // attribute is notified.
  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_EQ(1U, delegate()->get_changed_files().count(file_in_root));
}

TEST_F(DownloadOperationTest,
       EnsureFileDownloadedByPath_FromServer_NoSpaceAtAll) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));

  // Pretend we have no space at all.
  fake_free_disk_space_getter()->set_default_value(0);

  FileError error = FILE_ERROR_OK;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_NO_LOCAL_SPACE, error);
}

TEST_F(DownloadOperationTest,
       EnsureFileDownloadedByPath_FromServer_NoEnoughSpaceButCanFreeUp) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  // Make another file cached.
  // This file's cache file will be removed to free up the disk space.
  base::FilePath cached_file(
      FILE_PATH_LITERAL("drive/root/Duplicate Name.txt"));
  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      cached_file,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->file_specific_info().cache_state().is_present());

  // Pretend we have no space first (checked before downloading a file),
  // but then start reporting we have space. This is to emulate that
  // the disk space was freed up by removing temporary files.
  fake_free_disk_space_getter()->set_default_value(
      file_size + drive::internal::kMinFreeSpaceInBytes);
  fake_free_disk_space_getter()->PushFakeValue(0);
  fake_free_disk_space_getter()->PushFakeValue(0);

  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->file_specific_info().is_hosted_document());

  // The transfered file is cached and the change of "offline available"
  // attribute is notified.
  EXPECT_EQ(2U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(file_in_root));
  EXPECT_TRUE(delegate()->get_changed_files().count(cached_file));

  // The cache for the other file should be removed in order to free up space.
  ResourceEntry cached_file_entry;
  EXPECT_EQ(FILE_ERROR_OK,
            GetLocalResourceEntry(cached_file, &cached_file_entry));
  EXPECT_FALSE(
      cached_file_entry.file_specific_info().cache_state().is_present());
}

TEST_F(DownloadOperationTest,
       EnsureFileDownloadedByPath_FromServer_EnoughSpaceButBecomeFull) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));
  const int64_t file_size = src_entry.file_info().size();

  // Pretend we have enough space first (checked before downloading a file),
  // but then start reporting we have not enough space. This is to emulate that
  // the disk space becomes full after the file is downloaded for some reason
  // (ex. the actual file was larger than the expected size).
  fake_free_disk_space_getter()->PushFakeValue(
      file_size + drive::internal::kMinFreeSpaceInBytes);
  fake_free_disk_space_getter()->set_default_value(
      drive::internal::kMinFreeSpaceInBytes - 1);

  FileError error = FILE_ERROR_OK;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_NO_LOCAL_SPACE, error);
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByPath_FromCache) {
  base::FilePath temp_file;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir(), &temp_file));

  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  // Store something as cached version of this file.
  FileError error = FILE_ERROR_OK;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::FileCache::Store,
                 base::Unretained(cache()),
                 GetLocalId(file_in_root),
                 src_entry.file_specific_info().md5(),
                 temp_file,
                 internal::FileCache::FILE_OPERATION_COPY),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->file_specific_info().is_hosted_document());
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByPath_HostedDocument) {
  base::FilePath file_in_root(FILE_PATH_LITERAL(
      "drive/root/Document 1 excludeDir-test.gdoc"));

  FileError error = FILE_ERROR_FAILED;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->file_specific_info().is_hosted_document());
  EXPECT_FALSE(file_path.empty());

  EXPECT_EQ(GURL(entry->alternate_url()), util::ReadUrlFromGDocFile(file_path));
  EXPECT_EQ(entry->resource_id(), util::ReadResourceIdFromGDocFile(file_path));
  EXPECT_EQ(FILE_PATH_LITERAL(".gdoc"), file_path.Extension());
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByLocalId) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  FileError error = FILE_ERROR_OK;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByLocalId(
      GetLocalId(file_in_root),
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->file_specific_info().is_hosted_document());

  // The transfered file is cached and the change of "offline available"
  // attribute is notified.
  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_EQ(1U, delegate()->get_changed_files().count(file_in_root));
}

TEST_F(DownloadOperationTest,
       EnsureFileDownloadedByPath_WithGetContentCallback) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));

  {
    FileError initialized_error = FILE_ERROR_FAILED;
    std::unique_ptr<ResourceEntry> entry, entry_dontcare;
    base::FilePath local_path, local_path_dontcare;
    google_apis::test_util::TestGetContentCallback get_content_callback;
    FileError completion_error = FILE_ERROR_FAILED;
    base::Closure cancel_download = operation_->EnsureFileDownloadedByPath(
        file_in_root,
        ClientContext(USER_INITIATED),
        google_apis::test_util::CreateCopyResultCallback(
            &initialized_error, &local_path, &entry),
        get_content_callback.callback(),
        google_apis::test_util::CreateCopyResultCallback(
            &completion_error, &local_path_dontcare, &entry_dontcare));
    content::RunAllTasksUntilIdle();

    // For the first time, file is downloaded from the remote server.
    // In this case, |local_path| is empty.
    EXPECT_EQ(FILE_ERROR_OK, initialized_error);
    ASSERT_TRUE(entry);
    ASSERT_TRUE(local_path.empty());
    EXPECT_FALSE(cancel_download.is_null());
    // Content is available through the second callback argument.
    EXPECT_EQ(static_cast<size_t>(entry->file_info().size()),
              get_content_callback.GetConcatenatedData().size());
    EXPECT_EQ(FILE_ERROR_OK, completion_error);

    // The transfered file is cached and the change of "offline available"
    // attribute is notified.
    EXPECT_EQ(1U, delegate()->get_changed_files().size());
    EXPECT_EQ(1U, delegate()->get_changed_files().count(file_in_root));
  }

  {
    FileError initialized_error = FILE_ERROR_FAILED;
    std::unique_ptr<ResourceEntry> entry, entry_dontcare;
    base::FilePath local_path, local_path_dontcare;
    google_apis::test_util::TestGetContentCallback get_content_callback;
    FileError completion_error = FILE_ERROR_FAILED;
    base::Closure cancel_download = operation_->EnsureFileDownloadedByPath(
        file_in_root,
        ClientContext(USER_INITIATED),
        google_apis::test_util::CreateCopyResultCallback(
            &initialized_error, &local_path, &entry),
        get_content_callback.callback(),
        google_apis::test_util::CreateCopyResultCallback(
            &completion_error, &local_path_dontcare, &entry_dontcare));
    content::RunAllTasksUntilIdle();

    // Try second download. In this case, the file should be cached, so
    // |local_path| should not be empty.
    EXPECT_EQ(FILE_ERROR_OK, initialized_error);
    ASSERT_TRUE(entry);
    ASSERT_TRUE(!local_path.empty());
    EXPECT_FALSE(cancel_download.is_null());
    // The content is available from the cache file.
    EXPECT_TRUE(get_content_callback.data().empty());
    int64_t local_file_size = 0;
    base::GetFileSize(local_path, &local_file_size);
    EXPECT_EQ(entry->file_info().size(), local_file_size);
    EXPECT_EQ(FILE_ERROR_OK, completion_error);
  }
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByLocalId_FromCache) {
  base::FilePath temp_file;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir(), &temp_file));

  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  // Store something as cached version of this file.
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::FileCache::Store,
                 base::Unretained(cache()),
                 GetLocalId(file_in_root),
                 src_entry.file_specific_info().md5(),
                 temp_file,
                 internal::FileCache::FILE_OPERATION_COPY),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // The file is obtained from the cache.
  // Hence the downloading should work even if the drive service is offline.
  fake_service()->set_offline(true);

  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByLocalId(
      GetLocalId(file_in_root),
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->file_specific_info().is_hosted_document());
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByPath_DirtyCache) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  // Prepare a dirty file to store to cache that has a different size than
  // stored in resource metadata.
  base::FilePath dirty_file = temp_dir().AppendASCII("dirty.txt");
  size_t dirty_size = src_entry.file_info().size() + 10;
  google_apis::test_util::WriteStringToFile(dirty_file,
                                            std::string(dirty_size, 'x'));

  // Store the file as a cache, marking it to be dirty.
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::FileCache::Store,
                 base::Unretained(cache()),
                 GetLocalId(file_in_root),
                 std::string(),
                 dirty_file,
                 internal::FileCache::FILE_OPERATION_COPY),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Record values passed to GetFileContentInitializedCallback().
  FileError init_error;
  base::FilePath init_path;
  std::unique_ptr<ResourceEntry> init_entry;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  base::Closure cancel_callback = operation_->EnsureFileDownloadedByPath(
      file_in_root,
      ClientContext(USER_INITIATED),
      google_apis::test_util::CreateCopyResultCallback(
          &init_error, &init_path, &init_entry),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  // Check that the result of local modification is propagated.
  EXPECT_EQ(static_cast<int64_t>(dirty_size), init_entry->file_info().size());
  EXPECT_EQ(static_cast<int64_t>(dirty_size), entry->file_info().size());
}

TEST_F(DownloadOperationTest, EnsureFileDownloadedByPath_LocallyCreatedFile) {
  // Add a new file with an empty resource ID.
  base::FilePath file_path(FILE_PATH_LITERAL("drive/root/New File.txt"));
  ResourceEntry parent;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_path.DirName(), &parent));

  ResourceEntry new_file;
  new_file.set_title("New File.txt");
  new_file.set_parent_local_id(parent.local_id());

  FileError error = FILE_ERROR_FAILED;
  std::string local_id;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::AddEntry,
                 base::Unretained(metadata()),
                 new_file,
                 &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  // Empty cache file should be returned.
  base::FilePath cache_file_path;
  std::unique_ptr<ResourceEntry> entry;
  operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &cache_file_path, &entry));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  int64_t cache_file_size = 0;
  EXPECT_TRUE(base::GetFileSize(cache_file_path, &cache_file_size));
  EXPECT_EQ(static_cast<int64_t>(0), cache_file_size);
  ASSERT_TRUE(entry);
  EXPECT_EQ(cache_file_size, entry->file_info().size());
}

TEST_F(DownloadOperationTest, CancelBeforeDownloadStarts) {
  base::FilePath file_in_root(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  ResourceEntry src_entry;
  ASSERT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(file_in_root, &src_entry));

  // Start operation.
  FileError error = FILE_ERROR_OK;
  base::FilePath file_path;
  std::unique_ptr<ResourceEntry> entry;
  base::Closure cancel_closure = operation_->EnsureFileDownloadedByLocalId(
      GetLocalId(file_in_root),
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      google_apis::test_util::CreateCopyResultCallback(
          &error, &file_path, &entry));

  // Cancel immediately.
  ASSERT_FALSE(cancel_closure.is_null());
  cancel_closure.Run();
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_ABORT, error);
}

}  // namespace file_system
}  // namespace drive
