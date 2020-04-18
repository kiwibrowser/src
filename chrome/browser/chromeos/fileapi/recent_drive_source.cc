// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_drive_source.h"

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/file_system_types.h"

using content::BrowserThread;

namespace chromeos {

namespace {

void OnGetMetadataOnIOThread(
    const storage::FileSystemOperation::GetMetadataCallback& callback,
    base::File::Error result,
    const base::File::Info& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(callback, result, info));
}

void GetMetadataOnIOThread(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const storage::FileSystemURL& url,
    int fields,
    const storage::FileSystemOperation::GetMetadataCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  file_system_context->operation_runner()->GetMetadata(
      url, fields, base::Bind(&OnGetMetadataOnIOThread, callback));
}

}  // namespace

const char RecentDriveSource::kLoadHistogramName[] =
    "FileBrowser.Recent.LoadDrive";

RecentDriveSource::RecentDriveSource(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RecentDriveSource::~RecentDriveSource() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RecentDriveSource::GetRecentFiles(Params params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!params_.has_value());
  DCHECK(files_.empty());
  DCHECK_EQ(0, num_inflight_stats_);
  DCHECK(build_start_time_.is_null());

  params_.emplace(std::move(params));

  build_start_time_ = base::TimeTicks::Now();

  drive::FileSystemInterface* file_system =
      drive::util::GetFileSystemByProfile(profile_);
  if (!file_system) {
    // |file_system| is nullptr if Drive is disabled.
    OnSearchMetadata(drive::FILE_ERROR_FAILED, nullptr);
    return;
  }

  file_system->SearchMetadata(
      "" /* query */, drive::SEARCH_METADATA_EXCLUDE_DIRECTORIES,
      params_.value().max_files(), drive::MetadataSearchOrder::LAST_MODIFIED,
      base::Bind(&RecentDriveSource::OnSearchMetadata,
                 weak_ptr_factory_.GetWeakPtr()));
}

void RecentDriveSource::OnSearchMetadata(
    drive::FileError error,
    std::unique_ptr<drive::MetadataSearchResultVector> results) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(params_.has_value());
  DCHECK(files_.empty());
  DCHECK_EQ(0, num_inflight_stats_);
  DCHECK(!build_start_time_.is_null());

  if (error != drive::FILE_ERROR_OK) {
    OnComplete();
    return;
  }

  DCHECK(results.get());

  std::string extension_id = params_.value().origin().host();

  for (const auto& result : *results) {
    if (result.is_directory)
      continue;

    base::FilePath virtual_path =
        file_manager::util::ConvertDrivePathToRelativeFileSystemPath(
            profile_, extension_id, result.path);
    storage::FileSystemURL url =
        params_.value().file_system_context()->CreateCrackedFileSystemURL(
            params_.value().origin(), storage::kFileSystemTypeExternal,
            virtual_path);
    ++num_inflight_stats_;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &GetMetadataOnIOThread,
            base::WrapRefCounted(params_.value().file_system_context()), url,
            storage::FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED,
            base::Bind(&RecentDriveSource::OnGetMetadata,
                       weak_ptr_factory_.GetWeakPtr(), url)));
  }

  if (num_inflight_stats_ == 0)
    OnComplete();
}

void RecentDriveSource::OnGetMetadata(const storage::FileSystemURL& url,
                                      base::File::Error result,
                                      const base::File::Info& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (result == base::File::FILE_OK)
    files_.emplace_back(url, info.last_modified);

  --num_inflight_stats_;
  if (num_inflight_stats_ == 0)
    OnComplete();
}

void RecentDriveSource::OnComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(params_.has_value());
  DCHECK_EQ(0, num_inflight_stats_);
  DCHECK(!build_start_time_.is_null());

  UMA_HISTOGRAM_TIMES(kLoadHistogramName,
                      base::TimeTicks::Now() - build_start_time_);
  build_start_time_ = base::TimeTicks();

  Params params = std::move(params_.value());
  params_.reset();
  std::vector<RecentFile> files = std::move(files_);
  files_.clear();

  DCHECK(!params_.has_value());
  DCHECK(files_.empty());
  DCHECK_EQ(0, num_inflight_stats_);
  DCHECK(build_start_time_.is_null());

  std::move(params.callback()).Run(std::move(files));
}

}  // namespace chromeos
