// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/fake_drive_uploader.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "google_apis/drive/drive_api_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

using drive::FakeDriveService;
using drive::UploadCompletionCallback;
using google_apis::CancelCallback;
using google_apis::FileResource;
using google_apis::FileResourceCallback;
using google_apis::DriveApiErrorCode;
using google_apis::ProgressCallback;

namespace sync_file_system {
namespace drive_backend {

namespace {

void DidAddFileOrDirectoryForMakingConflict(
    DriveApiErrorCode error,
    std::unique_ptr<FileResource> entry) {
  ASSERT_EQ(google_apis::HTTP_CREATED, error);
  ASSERT_TRUE(entry);
}

void DidAddFileForUploadNew(const UploadCompletionCallback& callback,
                            DriveApiErrorCode error,
                            std::unique_ptr<FileResource> entry) {
  ASSERT_EQ(google_apis::HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback, google_apis::HTTP_SUCCESS, GURL(),
                                std::move(entry)));
}

void DidGetFileResourceForUploadExisting(
    const UploadCompletionCallback& callback,
    DriveApiErrorCode error,
    std::unique_ptr<FileResource> entry) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback, error, GURL(), std::move(entry)));
}

}  // namespace

FakeDriveServiceWrapper::FakeDriveServiceWrapper()
  : make_directory_conflict_(false) {}

FakeDriveServiceWrapper::~FakeDriveServiceWrapper() {}

CancelCallback FakeDriveServiceWrapper::AddNewDirectory(
    const std::string& parent_resource_id,
    const std::string& directory_name,
    const drive::AddNewDirectoryOptions& options,
    const FileResourceCallback& callback) {
  if (make_directory_conflict_) {
    FakeDriveService::AddNewDirectory(
        parent_resource_id,
        directory_name,
        options,
        base::Bind(&DidAddFileOrDirectoryForMakingConflict));
  }
  return FakeDriveService::AddNewDirectory(
      parent_resource_id, directory_name, options, callback);
}

FakeDriveUploader::FakeDriveUploader(
    FakeDriveServiceWrapper* fake_drive_service)
    : fake_drive_service_(fake_drive_service),
      make_file_conflict_(false) {}

FakeDriveUploader::~FakeDriveUploader() {}

void FakeDriveUploader::StartBatchProcessing() {
}

void FakeDriveUploader::StopBatchProcessing() {
}

CancelCallback FakeDriveUploader::UploadNewFile(
    const std::string& parent_resource_id,
    const base::FilePath& local_file_path,
    const std::string& title,
    const std::string& content_type,
    const drive::UploadNewFileOptions& options,
    const UploadCompletionCallback& callback,
    const ProgressCallback& progress_callback) {
  DCHECK(!callback.is_null());
  const std::string kFileContent = "test content";

  if (make_file_conflict_) {
    fake_drive_service_->AddNewFile(
        content_type,
        kFileContent,
        parent_resource_id,
        title,
        false,  // shared_with_me
        base::Bind(&DidAddFileOrDirectoryForMakingConflict));
  }

  fake_drive_service_->AddNewFile(
      content_type,
      kFileContent,
      parent_resource_id,
      title,
      false,  // shared_with_me
      base::Bind(&DidAddFileForUploadNew, callback));
  base::RunLoop().RunUntilIdle();

  return CancelCallback();
}

CancelCallback FakeDriveUploader::UploadExistingFile(
    const std::string& resource_id,
    const base::FilePath& local_file_path,
    const std::string& content_type,
    const drive::UploadExistingFileOptions& options,
    const UploadCompletionCallback& callback,
    const ProgressCallback& progress_callback) {
  DCHECK(!callback.is_null());
  return fake_drive_service_->GetFileResource(
      resource_id,
      base::Bind(&DidGetFileResourceForUploadExisting, callback));
}

CancelCallback FakeDriveUploader::ResumeUploadFile(
    const GURL& upload_location,
    const base::FilePath& local_file_path,
    const std::string& content_type,
    const drive::UploadCompletionCallback& callback,
    const ProgressCallback& progress_callback) {
  // At the moment, sync file system doesn't support resuming of the uploading.
  // So this method shouldn't be reached.
  NOTREACHED();
  return CancelCallback();
}

}  // namespace drive_backend
}  // namespace sync_file_system
