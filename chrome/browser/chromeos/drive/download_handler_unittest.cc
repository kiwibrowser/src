// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/download_handler.h"

#include <stdint.h>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/mock_download_item.h"
#include "components/drive/chromeos/dummy_file_system.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/test/mock_download_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

namespace {

// Test file system for verifying the behavior of DownloadHandler, by simulating
// various responses from FileSystem.
class DownloadHandlerTestFileSystem : public DummyFileSystem {
 public:
  DownloadHandlerTestFileSystem() : error_(FILE_ERROR_FAILED) {}

  void set_error(FileError error) { error_ = error; }

  // FileSystemInterface overrides.
  void GetResourceEntry(const base::FilePath& file_path,
                        const GetResourceEntryCallback& callback) override {
    callback.Run(error_, std::unique_ptr<ResourceEntry>(error_ == FILE_ERROR_OK
                                                            ? new ResourceEntry
                                                            : NULL));
  }

  void CreateDirectory(const base::FilePath& directory_path,
                       bool is_exclusive,
                       bool is_recursive,
                       const FileOperationCallback& callback) override {
    callback.Run(error_);
  }

  void FreeDiskSpaceIfNeededFor(
      int64_t num_bytes,
      const FreeDiskSpaceCallback& callback) override {
    free_disk_space_if_needed_for_num_bytes_.push_back(num_bytes);
    callback.Run(true);
  }

  std::vector<int64_t> free_disk_space_if_needed_for_num_bytes_;

 private:
  FileError error_;
};

class DownloadHandlerTestDownloadManager : public content::MockDownloadManager {
 public:
  void GetAllDownloads(
      content::DownloadManager::DownloadVector* downloads) override {
    for (auto* test_download : test_downloads_) {
      downloads->push_back(test_download);
    }
  }

  content::DownloadManager::DownloadVector test_downloads_;
};

class DownloadHandlerTestDownloadItem : public download::MockDownloadItem {
 public:
  bool IsDone() const override { return is_done_; }

  int64_t GetTotalBytes() const override { return total_bytes_; }

  int64_t GetReceivedBytes() const override { return received_bytes_; }

  bool is_done_ = false;
  int64_t total_bytes_ = 0;
  int64_t received_bytes_ = 0;
};

}  // namespace

class DownloadHandlerTest : public testing::Test {
 public:
  DownloadHandlerTest()
      : download_manager_(new DownloadHandlerTestDownloadManager) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // Set expectations for download item.
    EXPECT_CALL(download_item_, GetState())
        .WillRepeatedly(testing::Return(download::DownloadItem::IN_PROGRESS));

    download_handler_.reset(new DownloadHandler(&test_file_system_));
    download_handler_->Initialize(download_manager_.get(), temp_dir_.GetPath());
    download_handler_->SetFreeDiskSpaceDelayForTesting(
        base::TimeDelta::FromMilliseconds(0));
  }

 protected:
  base::ScopedTempDir temp_dir_;
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  std::unique_ptr<DownloadHandlerTestDownloadManager> download_manager_;
  std::unique_ptr<DownloadHandlerTestDownloadManager>
      incognito_download_manager_;
  DownloadHandlerTestFileSystem test_file_system_;
  std::unique_ptr<DownloadHandler> download_handler_;
  download::MockDownloadItem download_item_;
};

TEST_F(DownloadHandlerTest, SubstituteDriveDownloadPathNonDrivePath) {
  const base::FilePath non_drive_path(FILE_PATH_LITERAL("/foo/bar"));
  ASSERT_FALSE(util::IsUnderDriveMountPoint(non_drive_path));

  // Call SubstituteDriveDownloadPath()
  base::FilePath substituted_path;
  download_handler_->SubstituteDriveDownloadPath(
      non_drive_path,
      &download_item_,
      google_apis::test_util::CreateCopyResultCallback(&substituted_path));
  content::RunAllTasksUntilIdle();

  // Check the result.
  EXPECT_EQ(non_drive_path, substituted_path);
  EXPECT_FALSE(download_handler_->IsDriveDownload(&download_item_));
}

TEST_F(DownloadHandlerTest, SubstituteDriveDownloadPath) {
  const base::FilePath drive_path =
      util::GetDriveMountPointPath(&profile_).AppendASCII("test.dat");

  // Test the case that the download target directory already exists.
  test_file_system_.set_error(FILE_ERROR_OK);

  // Call SubstituteDriveDownloadPath()
  base::FilePath substituted_path;
  download_handler_->SubstituteDriveDownloadPath(
      drive_path,
      &download_item_,
      google_apis::test_util::CreateCopyResultCallback(&substituted_path));
  content::RunAllTasksUntilIdle();

  // Check the result.
  EXPECT_TRUE(temp_dir_.GetPath().IsParent(substituted_path));
  ASSERT_TRUE(download_handler_->IsDriveDownload(&download_item_));
  EXPECT_EQ(drive_path, download_handler_->GetTargetPath(&download_item_));
}

TEST_F(DownloadHandlerTest, SubstituteDriveDownloadPathGetEntryFailure) {
  const base::FilePath drive_path =
      util::GetDriveMountPointPath(&profile_).AppendASCII("test.dat");

  // Test the case that access to the download target directory failed for some
  // reason.
  test_file_system_.set_error(FILE_ERROR_FAILED);

  // Call SubstituteDriveDownloadPath()
  base::FilePath substituted_path;
  download_handler_->SubstituteDriveDownloadPath(
      drive_path,
      &download_item_,
      google_apis::test_util::CreateCopyResultCallback(&substituted_path));
  content::RunAllTasksUntilIdle();

  // Check the result.
  EXPECT_TRUE(substituted_path.empty());
}

// content::SavePackage calls SubstituteDriveDownloadPath before creating
// DownloadItem.
TEST_F(DownloadHandlerTest, SubstituteDriveDownloadPathForSavePackage) {
  const base::FilePath drive_path =
      util::GetDriveMountPointPath(&profile_).AppendASCII("test.dat");
  test_file_system_.set_error(FILE_ERROR_OK);

  // Call SubstituteDriveDownloadPath()
  base::FilePath substituted_path;
  download_handler_->SubstituteDriveDownloadPath(
      drive_path,
      NULL,  // DownloadItem is not available at this moment.
      google_apis::test_util::CreateCopyResultCallback(&substituted_path));
  content::RunAllTasksUntilIdle();

  // Check the result of SubstituteDriveDownloadPath().
  EXPECT_TRUE(temp_dir_.GetPath().IsParent(substituted_path));

  // |download_item_| is not a drive download yet.
  EXPECT_FALSE(download_handler_->IsDriveDownload(&download_item_));

  // Call SetDownloadParams().
  download_handler_->SetDownloadParams(drive_path, &download_item_);

  // |download_item_| is a drive download now.
  ASSERT_TRUE(download_handler_->IsDriveDownload(&download_item_));
  EXPECT_EQ(drive_path, download_handler_->GetTargetPath(&download_item_));
}

TEST_F(DownloadHandlerTest, CheckForFileExistence) {
  const base::FilePath drive_path =
      util::GetDriveMountPointPath(&profile_).AppendASCII("test.dat");

  // Make |download_item_| a drive download.
  download_handler_->SetDownloadParams(drive_path, &download_item_);
  ASSERT_TRUE(download_handler_->IsDriveDownload(&download_item_));
  EXPECT_EQ(drive_path, download_handler_->GetTargetPath(&download_item_));

  // Test for the case when the path exists.
  test_file_system_.set_error(FILE_ERROR_OK);

  // Call CheckForFileExistence.
  bool file_exists = false;
  download_handler_->CheckForFileExistence(
      &download_item_,
      google_apis::test_util::CreateCopyResultCallback(&file_exists));
  content::RunAllTasksUntilIdle();

  // Check the result.
  EXPECT_TRUE(file_exists);

  // Test for the case when the path does not exist.
  test_file_system_.set_error(FILE_ERROR_NOT_FOUND);

  // Call CheckForFileExistence again.
  download_handler_->CheckForFileExistence(
      &download_item_,
      google_apis::test_util::CreateCopyResultCallback(&file_exists));
  content::RunAllTasksUntilIdle();

  // Check the result.
  EXPECT_FALSE(file_exists);
}

TEST_F(DownloadHandlerTest, FreeDiskSpace) {
  // Add a download item to download manager.
  DownloadHandlerTestDownloadItem download_item_a;
  download_item_a.is_done_ = false;
  download_item_a.total_bytes_ = 100;
  download_item_a.received_bytes_ = 10;
  download_manager_->test_downloads_.push_back(&download_item_a);

  // Free disk space for download_item_a.
  download_handler_->FreeDiskSpaceIfNeededImmediately();
  ASSERT_EQ(1u,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_.size());
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_[0]);

  // Confirm that FreeDiskSpaceIfNeeded is rate limited by calling it twice.
  download_handler_->FreeDiskSpaceIfNeeded();
  download_handler_->FreeDiskSpaceIfNeeded();
  ASSERT_EQ(1u,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_.size());

  download_item_a.received_bytes_ = 20;

  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(2u,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_.size());
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_[1]);

  // Observe incognito download manager and add another download item.
  // FreeDiskSpace should be called with considering both download items.
  incognito_download_manager_.reset(new DownloadHandlerTestDownloadManager);
  download_handler_->ObserveIncognitoDownloadManager(
      incognito_download_manager_.get());

  DownloadHandlerTestDownloadItem download_item_b;
  download_item_b.is_done_ = false;
  download_item_b.total_bytes_ = 200;
  download_item_b.received_bytes_ = 0;
  incognito_download_manager_->test_downloads_.push_back(&download_item_b);

  download_item_a.received_bytes_ = 30;

  download_handler_->FreeDiskSpaceIfNeededImmediately();
  ASSERT_EQ(3u,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_.size());
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_ +
                download_item_b.total_bytes_ - download_item_b.received_bytes_,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_[2]);

  // Free disk space after making both items completed. In this case
  // FreeDiskSpace should be called with 0 byte to keep
  // drive::internal::kMinFreeSpaceInBytes.
  download_item_a.is_done_ = true;
  download_item_b.is_done_ = true;

  download_handler_->FreeDiskSpaceIfNeeded();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(4u,
            test_file_system_.free_disk_space_if_needed_for_num_bytes_.size());
  ASSERT_EQ(0, test_file_system_.free_disk_space_if_needed_for_num_bytes_[3]);
}

TEST_F(DownloadHandlerTest, CalculateRequestSpace) {
  DownloadHandlerTestDownloadItem download_item_a;
  download_item_a.is_done_ = false;
  download_item_a.total_bytes_ = 100;
  download_item_a.received_bytes_ = 0;

  DownloadHandlerTestDownloadItem download_item_b;
  download_item_b.is_done_ = false;
  download_item_b.total_bytes_ = 200;
  download_item_b.received_bytes_ = 10;

  content::DownloadManager::DownloadVector downloads;
  downloads.push_back(&download_item_a);
  downloads.push_back(&download_item_b);

  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_ +
                download_item_b.total_bytes_ - download_item_b.received_bytes_,
            download_handler_->CalculateRequestSpace(downloads));

  download_item_a.received_bytes_ = 10;

  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_ +
                download_item_b.total_bytes_ - download_item_b.received_bytes_,
            download_handler_->CalculateRequestSpace(downloads));

  download_item_b.is_done_ = true;

  // Since download_item_b is completed, it shouldn't be counted.
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_,
            download_handler_->CalculateRequestSpace(downloads));

  // Add unknown size download item.
  DownloadHandlerTestDownloadItem download_item_c;
  download_item_c.is_done_ = false;
  download_item_c.total_bytes_ = 0;
  downloads.push_back(&download_item_c);

  // Unknown size download should be counted as 0 byte.
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_,
            download_handler_->CalculateRequestSpace(downloads));

  // Add another unknown size download item.
  DownloadHandlerTestDownloadItem download_item_d;
  download_item_d.is_done_ = false;
  download_item_d.total_bytes_ = 0;
  downloads.push_back(&download_item_d);

  // Unknown size downloads should be counted as 0 byte.
  ASSERT_EQ(download_item_a.total_bytes_ - download_item_a.received_bytes_,
            download_handler_->CalculateRequestSpace(downloads));
}

}  // namespace drive
