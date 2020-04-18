// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/start_page_token_loader.h"

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/event_logger.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/resource_metadata_storage.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class StartPageTokenLoaderTest : public testing::Test {
 protected:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        google_apis::kEnableTeamDrives);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    pref_service_.reset(new TestingPrefServiceSimple);
    test_util::RegisterDrivePrefs(pref_service_->registry());

    logger_.reset(new EventLogger);

    drive_service_.reset(new FakeDriveService);
    ASSERT_TRUE(test_util::SetUpTestEntries(drive_service_.get()));

    scheduler_.reset(new JobScheduler(
        pref_service_.get(), logger_.get(), drive_service_.get(),
        base::ThreadTaskRunnerHandle::Get().get(), nullptr));
    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.GetPath(), base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_TRUE(metadata_storage_->Initialize());

    cache_.reset(new FileCache(metadata_storage_.get(), temp_dir_.GetPath(),
                               base::ThreadTaskRunnerHandle::Get().get(),
                               NULL /* free_disk_space_getter */));
    ASSERT_TRUE(cache_->Initialize());

    metadata_.reset(
        new ResourceMetadata(metadata_storage_.get(), cache_.get(),
                             base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_EQ(FILE_ERROR_OK, metadata_->Initialize());

    start_page_token_loader_.reset(
        new StartPageTokenLoader(empty_team_drive_id_, scheduler_.get()));
  }

  // Adds a new file to the root directory of the service.
  std::unique_ptr<google_apis::FileResource> AddNewFile(
      const std::string& title) {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_FILE_ERROR;
    std::unique_ptr<google_apis::FileResource> entry;
    drive_service_->AddNewFile(
        "text/plain", "content text", drive_service_->GetRootResourceId(),
        title,
        false,  // shared_with_me
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(google_apis::HTTP_CREATED, error);
    return entry;
  }

  // Empty team drive id equates to the users default corpus.
  const std::string empty_team_drive_id_;
  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<EventLogger> logger_;
  std::unique_ptr<FakeDriveService> drive_service_;
  std::unique_ptr<JobScheduler> scheduler_;
  std::unique_ptr<ResourceMetadataStorage, test_util::DestroyHelperForTests>
      metadata_storage_;
  std::unique_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
  std::unique_ptr<ResourceMetadata, test_util::DestroyHelperForTests> metadata_;
  std::unique_ptr<StartPageTokenLoader> start_page_token_loader_;
};

TEST_F(StartPageTokenLoaderTest, SingleStartPageTokenLoader) {
  google_apis::DriveApiErrorCode error[6] = {};
  std::unique_ptr<google_apis::StartPageToken> start_page_token[6];

  EXPECT_EQ(0, drive_service_->start_page_token_load_count());

  ASSERT_FALSE(start_page_token_loader_->cached_start_page_token());

  start_page_token_loader_->GetStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 0,
                                                       start_page_token + 0));
  start_page_token_loader_->GetStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 1,
                                                       start_page_token + 1));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, error[0]);
  EXPECT_EQ(google_apis::HTTP_SUCCESS, error[1]);
  const std::string& first_token = start_page_token[0]->start_page_token();
  EXPECT_EQ(first_token, start_page_token[1]->start_page_token());
  ASSERT_TRUE(start_page_token_loader_->cached_start_page_token());
  ASSERT_EQ(
      first_token,
      start_page_token_loader_->cached_start_page_token()->start_page_token());

  // Create a new change, which should change the start page token.
  AddNewFile("temp");

  start_page_token_loader_->UpdateStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 2,
                                                       start_page_token + 2));
  start_page_token_loader_->GetStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 3,
                                                       start_page_token + 3));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, error[2]);
  EXPECT_EQ(google_apis::HTTP_SUCCESS, error[3]);
  const std::string& second_token = start_page_token[2]->start_page_token();
  EXPECT_NE(first_token, second_token);
  EXPECT_EQ(second_token, start_page_token[3]->start_page_token());
  ASSERT_EQ(
      second_token,
      start_page_token_loader_->cached_start_page_token()->start_page_token());

  // Create a new change, which should change the start page token.
  AddNewFile("temp2");

  start_page_token_loader_->GetStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 4,
                                                       start_page_token + 4));
  start_page_token_loader_->UpdateStartPageToken(
      google_apis::test_util::CreateCopyResultCallback(error + 5,
                                                       start_page_token + 5));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT, error[4]);
  EXPECT_EQ(google_apis::HTTP_SUCCESS, error[5]);
  EXPECT_EQ(second_token, start_page_token[4]->start_page_token());
  const std::string& third_token = start_page_token[5]->start_page_token();
  EXPECT_NE(first_token, third_token);
  EXPECT_NE(second_token, third_token);
  ASSERT_EQ(
      third_token,
      start_page_token_loader_->cached_start_page_token()->start_page_token());

  EXPECT_EQ(3, drive_service_->start_page_token_load_count());
}

}  // namespace internal
}  // namespace drive
