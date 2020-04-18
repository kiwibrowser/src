// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/fake_file_system.h"

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace test_util {

class FakeFileSystemTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize FakeDriveService.
    fake_drive_service_.reset(new FakeDriveService);
    ASSERT_TRUE(SetUpTestEntries(fake_drive_service_.get()));

    // Create a testee instance.
    fake_file_system_.reset(new FakeFileSystem(fake_drive_service_.get()));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<FakeDriveService> fake_drive_service_;
  std::unique_ptr<FakeFileSystem> fake_file_system_;
};

TEST_F(FakeFileSystemTest, GetFileContent) {
  FileError initialize_error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  base::FilePath cache_file_path;
  google_apis::test_util::TestGetContentCallback get_content_callback;
  FileError completion_error = FILE_ERROR_FAILED;

  const base::FilePath kDriveFile =
      util::GetDriveMyDriveRootPath().AppendASCII("File 1.txt");

  // For the first time, the file should be downloaded from the service.
  base::Closure cancel_download = fake_file_system_->GetFileContent(
      kDriveFile,
      google_apis::test_util::CreateCopyResultCallback(
          &initialize_error, &cache_file_path, &entry),
      get_content_callback.callback(),
      google_apis::test_util::CreateCopyResultCallback(&completion_error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, initialize_error);
  EXPECT_TRUE(entry);

  // No cache file is available yet.
  EXPECT_TRUE(cache_file_path.empty());

  // The download should be happened so the |get_content_callback|
  // should have the actual data.
  std::string content = get_content_callback.GetConcatenatedData();
  EXPECT_EQ(26U, content.size());
  EXPECT_EQ(FILE_ERROR_OK, completion_error);

  initialize_error = FILE_ERROR_FAILED;
  entry.reset();
  get_content_callback.mutable_data()->clear();
  completion_error = FILE_ERROR_FAILED;

  // For the second time, the cache file should be found.
  cancel_download = fake_file_system_->GetFileContent(
      kDriveFile,
      google_apis::test_util::CreateCopyResultCallback(
          &initialize_error, &cache_file_path, &entry),
      get_content_callback.callback(),
      google_apis::test_util::CreateCopyResultCallback(&completion_error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, initialize_error);
  EXPECT_TRUE(entry);

  // Cache file should be available.
  ASSERT_FALSE(cache_file_path.empty());

  // There should be a cache file so no data should be downloaded.
  EXPECT_TRUE(get_content_callback.data().empty());
  EXPECT_EQ(FILE_ERROR_OK, completion_error);

  // Make sure the cached file's content.
  std::string cache_file_content;
  ASSERT_TRUE(
      base::ReadFileToString(cache_file_path, &cache_file_content));
  EXPECT_EQ(content, cache_file_content);
}

TEST_F(FakeFileSystemTest, GetFileContent_Directory) {
  FileError initialize_error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  base::FilePath cache_file_path;
  google_apis::test_util::TestGetContentCallback get_content_callback;
  FileError completion_error = FILE_ERROR_FAILED;
  base::Closure cancel_download = fake_file_system_->GetFileContent(
      util::GetDriveMyDriveRootPath(),
      google_apis::test_util::CreateCopyResultCallback(
          &initialize_error, &cache_file_path, &entry),
      get_content_callback.callback(),
      google_apis::test_util::CreateCopyResultCallback(&completion_error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(FILE_ERROR_NOT_A_FILE, completion_error);
}

TEST_F(FakeFileSystemTest, GetResourceEntry) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  fake_file_system_->GetResourceEntry(
      util::GetDriveMyDriveRootPath().AppendASCII(
          "Directory 1/Sub Directory Folder"),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ("sub_dir_folder_resource_id", entry->resource_id());
}

TEST_F(FakeFileSystemTest, GetResourceEntry_Root) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  fake_file_system_->GetResourceEntry(
      util::GetDriveMyDriveRootPath(),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->file_info().is_directory());
  EXPECT_EQ(fake_drive_service_->GetRootResourceId(), entry->resource_id());
  EXPECT_EQ(util::kDriveMyDriveRootDirName, entry->title());
}

TEST_F(FakeFileSystemTest, GetResourceEntry_Invalid) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  fake_file_system_->GetResourceEntry(
      util::GetDriveMyDriveRootPath().AppendASCII("Invalid File Name"),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(FILE_ERROR_NOT_FOUND, error);
  ASSERT_FALSE(entry);
}

}  // namespace test_util
}  // namespace drive
