// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/team_drive_list_loader.h"

#include "base/files/scoped_temp_dir.h"
#include "components/drive/chromeos/change_list_loader.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/event_logger.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class TeamDriveListLoaderTest : public testing::Test {
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
    drive_service_->set_default_max_results(2);

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

    loader_controller_.reset(new LoaderController);
    team_drive_list_loader_.reset(new TeamDriveListLoader(
        logger_.get(), base::ThreadTaskRunnerHandle::Get().get(),
        metadata_.get(), scheduler_.get(), loader_controller_.get()));
  }

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
  std::unique_ptr<LoaderController> loader_controller_;
  std::unique_ptr<TeamDriveListLoader> team_drive_list_loader_;
};

TEST_F(TeamDriveListLoaderTest, NoTeamDrives) {
  // Tests the response if there are no team drives loaded.
  FileError error = FILE_ERROR_FAILED;
  team_drive_list_loader_->LoadIfNeeded(
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(1, drive_service_->team_drive_list_load_count());

  ResourceEntryVector entries;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->ReadDirectoryByPath(
                               util::GetDriveTeamDrivesRootPath(), &entries));
  EXPECT_TRUE(entries.empty());
}

TEST_F(TeamDriveListLoaderTest, OneTeamDrive) {
  constexpr char kTeamDriveId1[] = "the1stTeamDriveId";
  constexpr char kTeamDriveName1[] = "The First Team Drive";
  drive_service_->AddTeamDrive(kTeamDriveId1, kTeamDriveName1);

  FileError error = FILE_ERROR_FAILED;
  team_drive_list_loader_->CheckForUpdates(
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(1, drive_service_->team_drive_list_load_count());

  ResourceEntryVector entries;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->ReadDirectoryByPath(
                               util::GetDriveTeamDrivesRootPath(), &entries));
  EXPECT_EQ(1UL, entries.size());

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(
                util::GetDriveTeamDrivesRootPath().AppendASCII(kTeamDriveName1),
                &entry));
}

TEST_F(TeamDriveListLoaderTest, MultipleTeamDrive) {
  const std::vector<std::pair<std::string, std::string>> team_drives = {
      {"the1stTeamDriveId", "The First Team Drive"},
      {"the2ndTeamDriveId", "The Second Team Drive"},
      {"the3rdTeamDriveId", "The Third Team Drive"},
      {"the4thTeamDriveId", "The Forth Team Drive"},
  };

  for (const auto& drive : team_drives) {
    drive_service_->AddTeamDrive(drive.first, drive.second);
  }

  FileError error = FILE_ERROR_FAILED;
  team_drive_list_loader_->CheckForUpdates(
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(1, drive_service_->team_drive_list_load_count());

  ResourceEntryVector entries;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->ReadDirectoryByPath(
                               util::GetDriveTeamDrivesRootPath(), &entries));
  EXPECT_EQ(team_drives.size(), entries.size());

  ResourceEntry entry;

  for (const auto& drive : team_drives) {
    EXPECT_EQ(FILE_ERROR_OK,
              metadata_->GetResourceEntryByPath(
                  util::GetDriveTeamDrivesRootPath().AppendASCII(drive.second),
                  &entry));
    EXPECT_EQ(drive.first, entry.resource_id());
    EXPECT_EQ(drive.second, entry.base_name());
    EXPECT_TRUE(entry.file_info().is_directory());
    EXPECT_EQ("", entry.directory_specific_info().start_page_token());
  }
}

}  // namespace internal
}  // namespace drive
