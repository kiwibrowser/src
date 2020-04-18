// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/search_operation.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/resource_entry_conversion.h"
#include "google_apis/drive/drive_api_parser.h"
#include "url/gurl.h"

namespace drive {
namespace file_system {
namespace {

// Computes the path of each item in |file_list| returned from the server
// and stores to |result|, by using |resource_metadata|. If the metadata is not
// up to date and did not contain an item, adds the item to "drive/other" for
// temporally assigning a path.
FileError ResolveSearchResultOnBlockingPool(
    internal::ResourceMetadata* resource_metadata,
    std::unique_ptr<google_apis::FileList> file_list,
    std::vector<SearchResultInfo>* result) {
  DCHECK(resource_metadata);
  DCHECK(result);

  const std::vector<std::unique_ptr<google_apis::FileResource>>& entries =
      file_list->items();
  result->reserve(entries.size());
  for (size_t i = 0; i < entries.size(); ++i) {
    std::string local_id;
    FileError error = resource_metadata->GetIdByResourceId(
        entries[i]->file_id(), &local_id);

    ResourceEntry entry;
    if (error == FILE_ERROR_OK)
      error = resource_metadata->GetResourceEntryById(local_id, &entry);

    if (error == FILE_ERROR_NOT_FOUND) {
      std::string original_parent_id;
      if (!ConvertFileResourceToResourceEntry(*entries[i], &entry,
                                              &original_parent_id))
        continue;  // Skip non-file entries.

      // The result is absent in local resource metadata. This can happen if
      // the metadata is not synced to the latest server state yet. In that
      // case, we temporarily add the file to the special "drive/other"
      // directory in order to assign a path, which is needed to access the
      // file through FileSystem API.
      //
      // It will be moved to the right place when the metadata gets synced
      // in normal loading process in ChangeListProcessor.
      entry.set_parent_local_id(util::kDriveOtherDirLocalId);
      error = resource_metadata->AddEntry(entry, &local_id);
    }
    if (error != FILE_ERROR_OK)
      return error;
    base::FilePath path;
    error = resource_metadata->GetFilePath(local_id, &path);
    if (error != FILE_ERROR_OK)
      return error;
    result->push_back(SearchResultInfo(path, entry.file_info().is_directory()));
  }

  return FILE_ERROR_OK;
}

}  // namespace

SearchOperation::SearchOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    JobScheduler* scheduler,
    internal::ResourceMetadata* metadata,
    internal::LoaderController* loader_controller)
    : blocking_task_runner_(blocking_task_runner),
      scheduler_(scheduler),
      metadata_(metadata),
      loader_controller_(loader_controller),
      weak_ptr_factory_(this) {
}

SearchOperation::~SearchOperation() = default;

void SearchOperation::Search(const std::string& search_query,
                             const GURL& next_link,
                             const SearchCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (next_link.is_empty()) {
    // This is first request for the |search_query|.
    scheduler_->Search(
        search_query,
        base::Bind(&SearchOperation::SearchAfterGetFileList,
                   weak_ptr_factory_.GetWeakPtr(), callback));
  } else {
    // There is the remaining result so fetch it.
    scheduler_->GetRemainingFileList(
        next_link,
        base::Bind(&SearchOperation::SearchAfterGetFileList,
                   weak_ptr_factory_.GetWeakPtr(), callback));
  }
}

void SearchOperation::SearchAfterGetFileList(
    const SearchCallback& callback,
    google_apis::DriveApiErrorCode gdata_error,
    std::unique_ptr<google_apis::FileList> file_list) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  FileError error = GDataToFileError(gdata_error);
  if (error != FILE_ERROR_OK) {
    callback.Run(error, GURL(),
                 std::unique_ptr<std::vector<SearchResultInfo>>());
    return;
  }

  DCHECK(file_list);

  GURL next_url = file_list->next_link();

  std::unique_ptr<std::vector<SearchResultInfo>> result(
      new std::vector<SearchResultInfo>);
  if (file_list->items().empty()) {
    // Short cut. If the resource entry is empty, we don't need to refresh
    // the resource metadata.
    callback.Run(FILE_ERROR_OK, next_url, std::move(result));
    return;
  }

  // ResolveSearchResultOnBlockingPool() may add entries newly created on the
  // server to the local metadata.
  // This may race with sync tasks so we should ask LoaderController here.
  std::vector<SearchResultInfo>* result_ptr = result.get();
  loader_controller_->ScheduleRun(
      base::Bind(&drive::util::RunAsyncTask,
                 base::RetainedRef(blocking_task_runner_), FROM_HERE,
                 base::Bind(&ResolveSearchResultOnBlockingPool, metadata_,
                            base::Passed(&file_list), result_ptr),
                 base::Bind(&SearchOperation::SearchAfterResolveSearchResult,
                            weak_ptr_factory_.GetWeakPtr(), callback, next_url,
                            base::Passed(&result))));
}

void SearchOperation::SearchAfterResolveSearchResult(
    const SearchCallback& callback,
    const GURL& next_link,
    std::unique_ptr<std::vector<SearchResultInfo>> result,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  DCHECK(result);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, GURL(),
                 std::unique_ptr<std::vector<SearchResultInfo>>());
    return;
  }

  callback.Run(error, next_link, std::move(result));
}

}  // namespace file_system
}  // namespace drive
