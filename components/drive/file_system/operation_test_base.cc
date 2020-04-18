// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/file_system/operation_test_base.h"

#include "base/task_scheduler/post_task.h"
#include "components/drive/chromeos/about_resource_loader.h"
#include "components/drive/chromeos/about_resource_root_folder_id_loader.h"
#include "components/drive/chromeos/change_list_loader.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/chromeos/start_page_token_loader.h"
#include "components/drive/event_logger.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"

namespace drive {
namespace file_system {

OperationTestBase::LoggingDelegate::LoggingDelegate() = default;

OperationTestBase::LoggingDelegate::~LoggingDelegate() = default;

void OperationTestBase::LoggingDelegate::OnFileChangedByOperation(
    const FileChange& changed_files) {
  changed_files_.Apply(changed_files);
}

void OperationTestBase::LoggingDelegate::OnEntryUpdatedByOperation(
    const ClientContext& /* client_context */,
    const std::string& local_id) {
  updated_local_ids_.insert(local_id);
}

void OperationTestBase::LoggingDelegate::OnDriveSyncError(
    DriveSyncErrorType type, const std::string& local_id) {
  drive_sync_errors_.push_back(type);
}

bool OperationTestBase::LoggingDelegate::WaitForSyncComplete(
    const std::string& local_id,
    const FileOperationCallback& callback) {
  return wait_for_sync_complete_handler_.is_null() ?
      false : wait_for_sync_complete_handler_.Run(local_id, callback);
}

OperationTestBase::OperationTestBase() = default;

OperationTestBase::OperationTestBase(int test_thread_bundle_options)
    : thread_bundle_(test_thread_bundle_options) {
}

OperationTestBase::~OperationTestBase() = default;

void OperationTestBase::SetUp() {
  blocking_task_runner_ =
      base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()});

  pref_service_.reset(new TestingPrefServiceSimple);
  test_util::RegisterDrivePrefs(pref_service_->registry());

  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

  logger_.reset(new EventLogger);

  fake_drive_service_.reset(new FakeDriveService);
  ASSERT_TRUE(test_util::SetUpTestEntries(fake_drive_service_.get()));

  scheduler_.reset(new JobScheduler(pref_service_.get(), logger_.get(),
                                    fake_drive_service_.get(),
                                    blocking_task_runner_.get(), nullptr));

  metadata_storage_.reset(new internal::ResourceMetadataStorage(
      temp_dir_.GetPath(), blocking_task_runner_.get()));
  bool success = false;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadataStorage::Initialize,
                 base::Unretained(metadata_storage_.get())),
      google_apis::test_util::CreateCopyResultCallback(&success));
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(success);

  fake_free_disk_space_getter_.reset(new FakeFreeDiskSpaceGetter);
  cache_.reset(new internal::FileCache(
      metadata_storage_.get(), temp_dir_.GetPath(), blocking_task_runner_.get(),
      fake_free_disk_space_getter_.get()));
  success = false;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::FileCache::Initialize,
                 base::Unretained(cache_.get())),
      google_apis::test_util::CreateCopyResultCallback(&success));
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(success);

  metadata_.reset(new internal::ResourceMetadata(metadata_storage_.get(),
                                                 cache_.get(),
                                                 blocking_task_runner_));

  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::Initialize,
                 base::Unretained(metadata_.get())),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(FILE_ERROR_OK, error);

  // Makes sure the FakeDriveService's content is loaded to the metadata_.
  about_resource_loader_.reset(new internal::AboutResourceLoader(
      scheduler_.get()));
  root_folder_id_loader_ =
      std::make_unique<internal::AboutResourceRootFolderIdLoader>(
          about_resource_loader_.get());
  start_page_token_loader_.reset(new internal::StartPageTokenLoader(
      drive::util::kTeamDriveIdDefaultCorpus, scheduler_.get()));
  loader_controller_.reset(new internal::LoaderController);
  change_list_loader_.reset(new internal::ChangeListLoader(
      logger_.get(), blocking_task_runner_.get(), metadata_.get(),
      scheduler_.get(), root_folder_id_loader_.get(),
      start_page_token_loader_.get(), loader_controller_.get()));
  change_list_loader_->LoadIfNeeded(
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(FILE_ERROR_OK, error);
}

FileError OperationTestBase::GetLocalResourceEntry(const base::FilePath& path,
                                                   ResourceEntry* entry) {
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(), FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetResourceEntryByPath,
                 base::Unretained(metadata()), path, entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  return error;
}

FileError OperationTestBase::GetLocalResourceEntryById(
    const std::string& local_id,
    ResourceEntry* entry) {
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(), FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetResourceEntryById,
                 base::Unretained(metadata()), local_id, entry),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  return error;
}

std::string OperationTestBase::GetLocalId(const base::FilePath& path) {
  std::string local_id;
  FileError error = FILE_ERROR_FAILED;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner(), FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetIdByPath,
                 base::Unretained(metadata()), path, &local_id),
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error) << path.value();
  return local_id;
}

FileError OperationTestBase::CheckForUpdates() {
  FileError error = FILE_ERROR_FAILED;
  change_list_loader_->CheckForUpdates(
      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  return error;
}

}  // namespace file_system
}  // namespace drive
