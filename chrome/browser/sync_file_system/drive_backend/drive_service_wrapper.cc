// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/drive_service_wrapper.h"

#include <string>

#include "base/memory/weak_ptr.h"
#include "components/drive/service/drive_service_interface.h"

namespace sync_file_system {
namespace drive_backend {

DriveServiceWrapper::DriveServiceWrapper(
    drive::DriveServiceInterface* drive_service)
    : drive_service_(drive_service) {
  DCHECK(drive_service_);
}

void DriveServiceWrapper::AddNewDirectory(
    const std::string& parent_resource_id,
    const std::string& directory_title,
    const drive::AddNewDirectoryOptions& options,
    const google_apis::FileResourceCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->AddNewDirectory(parent_resource_id,
                                  directory_title,
                                  options,
                                  callback);
}

void DriveServiceWrapper::DeleteResource(
    const std::string& resource_id,
    const std::string& etag,
    const google_apis::EntryActionCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->DeleteResource(resource_id,
                                 etag,
                                 callback);
}

void DriveServiceWrapper::DownloadFile(
    const base::FilePath& local_cache_path,
    const std::string& resource_id,
    const google_apis::DownloadActionCallback& download_action_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const google_apis::ProgressCallback& progress_callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->DownloadFile(local_cache_path,
                               resource_id,
                               download_action_callback,
                               get_content_callback,
                               progress_callback);
}

void DriveServiceWrapper::GetAboutResource(
    const google_apis::AboutResourceCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetAboutResource(callback);
}

void DriveServiceWrapper::GetStartPageToken(
    const std::string& team_drive_id,
    const google_apis::StartPageTokenCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetStartPageToken(team_drive_id, callback);
}

void DriveServiceWrapper::GetChangeList(
    int64_t start_changestamp,
    const google_apis::ChangeListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetChangeList(start_changestamp, callback);
}

void DriveServiceWrapper::GetChangeListByToken(
    const std::string& team_drive_id,
    const std::string& start_page_token,
    const google_apis::ChangeListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetChangeListByToken(team_drive_id, start_page_token,
                                       callback);
}

void DriveServiceWrapper::GetRemainingChangeList(
    const GURL& next_link,
    const google_apis::ChangeListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetRemainingChangeList(next_link, callback);
}

void DriveServiceWrapper::GetRemainingTeamDriveList(
    const std::string& page_token,
    const google_apis::TeamDriveListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetRemainingTeamDriveList(page_token, callback);
}

void DriveServiceWrapper::GetRemainingFileList(
    const GURL& next_link,
    const google_apis::FileListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetRemainingFileList(next_link, callback);
}

void DriveServiceWrapper::GetFileResource(
    const std::string& resource_id,
    const google_apis::FileResourceCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetFileResource(resource_id, callback);
}

void DriveServiceWrapper::GetFileListInDirectory(
    const std::string& directory_resource_id,
    const google_apis::FileListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->GetFileListInDirectory(directory_resource_id, callback);
}

void DriveServiceWrapper::RemoveResourceFromDirectory(
    const std::string& parent_resource_id,
    const std::string& resource_id,
    const google_apis::EntryActionCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->RemoveResourceFromDirectory(
      parent_resource_id, resource_id, callback);
}

void DriveServiceWrapper::SearchByTitle(
    const std::string& title,
    const std::string& directory_resource_id,
    const google_apis::FileListCallback& callback) {
  DCHECK(sequece_checker_.CalledOnValidSequence());
  drive_service_->SearchByTitle(
      title, directory_resource_id, callback);
}

}  // namespace drive_backend
}  // namespace sync_file_system
