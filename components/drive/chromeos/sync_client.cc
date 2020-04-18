// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/sync_client.h"

#include <stddef.h>

#include <vector>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/sync/entry_update_performer.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "google_apis/drive/task_util.h"

namespace drive {
namespace internal {

namespace {

// The delay constant is used to delay processing a sync task. We should not
// process SyncTasks immediately for the following reasons:
//
// 1) For fetching, the user may accidentally click on "Make available
//    offline" checkbox on a file, and immediately cancel it in a second.
//    It's a waste to fetch the file in this case.
//
// 2) For uploading, file writing via HTML5 file system API is performed in
//    two steps: 1) truncate a file to 0 bytes, 2) write contents. We
//    shouldn't start uploading right after the step 1). Besides, the user
//    may edit the same file repeatedly in a short period of time.
//
// TODO(satorux): We should find a way to handle the upload case more nicely,
// and shorten the delay. crbug.com/134774
const int kDelaySeconds = 1;

// The delay constant is used to delay retrying a sync task on server errors.
const int kLongDelaySeconds = 600;

// Iterates entries and appends IDs to |to_fetch| if the file is pinned but not
// fetched (not present locally), to |to_update| if the file needs update.
void CollectBacklog(ResourceMetadata* metadata,
                    std::vector<std::string>* to_fetch,
                    std::vector<std::string>* to_update) {
  DCHECK(to_fetch);
  DCHECK(to_update);

  std::unique_ptr<ResourceMetadata::Iterator> it = metadata->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    const std::string& local_id = it->GetID();
    const ResourceEntry& entry = it->GetValue();
    if (entry.parent_local_id() == util::kDriveTrashDirLocalId) {
      to_update->push_back(local_id);
      continue;
    }

    bool should_update = false;
    switch (entry.metadata_edit_state()) {
      case ResourceEntry::CLEAN:
        break;
      case ResourceEntry::SYNCING:
      case ResourceEntry::DIRTY:
        should_update = true;
        break;
    }

    if (entry.file_specific_info().cache_state().is_pinned() &&
        !entry.file_specific_info().cache_state().is_present())
      to_fetch->push_back(local_id);

    if (entry.file_specific_info().cache_state().is_dirty())
      should_update = true;

    if (should_update)
      to_update->push_back(local_id);
  }
  DCHECK(!it->HasError());
}

// Iterates cache entries and collects IDs of ones with obsolete cache files.
void CheckExistingPinnedFiles(ResourceMetadata* metadata,
                              FileCache* cache,
                              std::vector<std::string>* local_ids) {
  std::unique_ptr<ResourceMetadata::Iterator> it = metadata->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    const ResourceEntry& entry = it->GetValue();
    const FileCacheEntry& cache_state =
        entry.file_specific_info().cache_state();
    const std::string& local_id = it->GetID();
    if (!cache_state.is_pinned() || !cache_state.is_present())
      continue;

    // If MD5s don't match, it indicates the local cache file is stale, unless
    // the file is dirty (the MD5 is "local"). We should never re-fetch the
    // file when we have a locally modified version.
    if (entry.file_specific_info().md5() == cache_state.md5() ||
        cache_state.is_dirty())
      continue;

    FileError error = cache->Remove(local_id);
    if (error != FILE_ERROR_OK) {
      LOG(WARNING) << "Failed to remove cache entry: " << local_id;
      continue;
    }

    error = cache->Pin(local_id);
    if (error != FILE_ERROR_OK) {
      LOG(WARNING) << "Failed to pin cache entry: " << local_id;
      continue;
    }

    local_ids->push_back(local_id);
  }
  DCHECK(!it->HasError());
}

// Gets the parent entry of the entry specified by the ID.
FileError GetParentResourceEntry(ResourceMetadata* metadata,
                                 const std::string& local_id,
                                 ResourceEntry* parent) {
  ResourceEntry entry;
  FileError error = metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  return metadata->GetResourceEntryById(entry.parent_local_id(), parent);
}

}  // namespace

SyncClient::SyncTask::SyncTask()
    : state(SUSPENDED), context(BACKGROUND), should_run_again(false) {}
SyncClient::SyncTask::SyncTask(const SyncTask& other) = default;
SyncClient::SyncTask::~SyncTask() = default;

SyncClient::SyncClient(base::SequencedTaskRunner* blocking_task_runner,
                       file_system::OperationDelegate* delegate,
                       JobScheduler* scheduler,
                       ResourceMetadata* metadata,
                       FileCache* cache,
                       LoaderController* loader_controller,
                       const base::FilePath& temporary_file_directory)
    : blocking_task_runner_(blocking_task_runner),
      operation_delegate_(delegate),
      metadata_(metadata),
      cache_(cache),
      download_operation_(new file_system::DownloadOperation(
          blocking_task_runner,
          delegate,
          scheduler,
          metadata,
          cache,
          temporary_file_directory)),
      entry_update_performer_(new EntryUpdatePerformer(blocking_task_runner,
                                                       delegate,
                                                       scheduler,
                                                       metadata,
                                                       cache,
                                                       loader_controller)),
      delay_(base::TimeDelta::FromSeconds(kDelaySeconds)),
      long_delay_(base::TimeDelta::FromSeconds(kLongDelaySeconds)),
      weak_ptr_factory_(this) {
}

SyncClient::~SyncClient() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void SyncClient::StartProcessingBacklog() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  std::vector<std::string>* to_fetch = new std::vector<std::string>;
  std::vector<std::string>* to_update = new std::vector<std::string>;
  blocking_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&CollectBacklog, metadata_, to_fetch, to_update),
      base::Bind(&SyncClient::OnGetLocalIdsOfBacklog,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(to_fetch),
                 base::Owned(to_update)));
}

void SyncClient::StartCheckingExistingPinnedFiles() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  std::vector<std::string>* local_ids = new std::vector<std::string>;
  blocking_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&CheckExistingPinnedFiles,
                 metadata_,
                 cache_,
                 local_ids),
      base::Bind(&SyncClient::AddFetchTasks,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(local_ids)));
}

void SyncClient::AddFetchTask(const std::string& local_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  AddFetchTaskInternal(local_id, delay_);
}

void SyncClient::RemoveFetchTask(const std::string& local_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  SyncTasks::iterator it = tasks_.find(SyncTasks::key_type(FETCH, local_id));
  if (it == tasks_.end())
    return;

  SyncTask* task = &it->second;
  switch (task->state) {
    case SUSPENDED:
    case PENDING:
      OnTaskComplete(FETCH, local_id, FILE_ERROR_ABORT);
      break;
    case RUNNING:
      if (task->cancel_closure)
        task->cancel_closure.Run();
      break;
  }
}

void SyncClient::AddUpdateTask(const ClientContext& context,
                               const std::string& local_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  AddUpdateTaskInternal(context, local_id, delay_);
}

bool SyncClient:: WaitForUpdateTaskToComplete(
    const std::string& local_id,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  SyncTasks::iterator it = tasks_.find(SyncTasks::key_type(UPDATE, local_id));
  if (it == tasks_.end())
    return false;

  SyncTask* task = &it->second;
  task->waiting_callbacks.push_back(callback);
  return true;
}

base::Closure SyncClient::PerformFetchTask(const std::string& local_id,
                                           const ClientContext& context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return download_operation_->EnsureFileDownloadedByLocalId(
      local_id,
      context,
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      base::Bind(&SyncClient::OnFetchFileComplete,
                 weak_ptr_factory_.GetWeakPtr(),
                 local_id));
}

void SyncClient::AddFetchTaskInternal(const std::string& local_id,
                                      const base::TimeDelta& delay) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  SyncTask task;
  task.state = PENDING;
  task.context = ClientContext(BACKGROUND);
  task.task = base::Bind(&SyncClient::PerformFetchTask,
                         base::Unretained(this),
                         local_id);
  AddTask(SyncTasks::key_type(FETCH, local_id), task, delay);
}

base::Closure SyncClient::PerformUpdateTask(const std::string& local_id,
                                            const ClientContext& context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  entry_update_performer_->UpdateEntry(
      local_id,
      context,
      base::Bind(&SyncClient::OnTaskComplete,
                 weak_ptr_factory_.GetWeakPtr(),
                 UPDATE,
                 local_id));
  return base::Closure();
}

void SyncClient::AddUpdateTaskInternal(const ClientContext& context,
                                       const std::string& local_id,
                                       const base::TimeDelta& delay) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  SyncTask task;
  task.state = PENDING;
  task.context = context;
  task.task = base::Bind(&SyncClient::PerformUpdateTask,
                         base::Unretained(this),
                         local_id);
  AddTask(SyncTasks::key_type(UPDATE, local_id), task, delay);
}

void SyncClient::AddTask(const SyncTasks::key_type& key,
                         const SyncTask& task,
                         const base::TimeDelta& delay) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  SyncTasks::iterator it = tasks_.find(key);
  if (it != tasks_.end()) {
    switch (it->second.state) {
      case SUSPENDED:
        // Activate the task.
        it->second.state = PENDING;
        break;
      case PENDING:
        // The same task will run, do nothing.
        return;
      case RUNNING:
        // Something has changed since the task started. Schedule rerun.
        it->second.should_run_again = true;
        return;
    }
  } else {
    tasks_[key] = task;
  }
  DCHECK_EQ(PENDING, task.state);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&SyncClient::StartTask, weak_ptr_factory_.GetWeakPtr(), key),
      delay);
}

void SyncClient::StartTask(const SyncTasks::key_type& key) {
  ResourceEntry* parent = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&GetParentResourceEntry, metadata_, key.second, parent),
      base::Bind(&SyncClient::StartTaskAfterGetParentResourceEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 key,
                 base::Owned(parent)));
}

void SyncClient::StartTaskAfterGetParentResourceEntry(
    const SyncTasks::key_type& key,
    const ResourceEntry* parent,
    FileError error) {
  const SyncType type = key.first;
  const std::string& local_id = key.second;
  SyncTasks::iterator it = tasks_.find(key);
  if (it == tasks_.end())
    return;

  SyncTask* task = &it->second;
  switch (task->state) {
    case SUSPENDED:
    case PENDING:
      break;
    case RUNNING:  // Do nothing.
      return;
  }

  if (error != FILE_ERROR_OK) {
    OnTaskComplete(type, local_id, error);
    return;
  }

  if (type == UPDATE &&
      parent->resource_id().empty() &&
      parent->local_id() != util::kDriveTrashDirLocalId) {
    // Parent entry needs to be synced to get a resource ID.
    // Suspend the task and register it as a dependent task of the parent.
    const SyncTasks::key_type key_parent(type, parent->local_id());
    SyncTasks::iterator it_parent = tasks_.find(key_parent);
    if (it_parent == tasks_.end()) {
      OnTaskComplete(type, local_id, FILE_ERROR_INVALID_OPERATION);
      LOG(WARNING) << "Parent task not found: type = " << type << ", id = "
                   << local_id << ", parent_id = " << parent->local_id();
      return;
    }
    task->state = SUSPENDED;
    it_parent->second.dependent_tasks.push_back(key);
    return;
  }

  // Run the task.
  task->state = RUNNING;
  task->cancel_closure = task->task.Run(task->context);
}

void SyncClient::OnGetLocalIdsOfBacklog(
    const std::vector<std::string>* to_fetch,
    const std::vector<std::string>* to_update) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Give priority to upload tasks over fetch tasks, so that dirty files are
  // uploaded as soon as possible.
  for (size_t i = 0; i < to_update->size(); ++i) {
    const std::string& local_id = (*to_update)[i];
    DVLOG(1) << "Queuing to update: " << local_id;
    AddUpdateTask(ClientContext(BACKGROUND), local_id);
  }

  for (size_t i = 0; i < to_fetch->size(); ++i) {
    const std::string& local_id = (*to_fetch)[i];
    DVLOG(1) << "Queuing to fetch: " << local_id;
    AddFetchTaskInternal(local_id, delay_);
  }
}

void SyncClient::AddFetchTasks(const std::vector<std::string>* local_ids) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (size_t i = 0; i < local_ids->size(); ++i)
    AddFetchTask((*local_ids)[i]);
}

void SyncClient::OnTaskComplete(SyncType type,
                                const std::string& local_id,
                                FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  const SyncTasks::key_type key(type, local_id);
  SyncTasks::iterator it = tasks_.find(key);
  DCHECK(it != tasks_.end());

  base::TimeDelta retry_delay = base::TimeDelta::FromSeconds(0);

  switch (error) {
    case FILE_ERROR_OK:
      DVLOG(1) << "Completed: type = " << type << ", id = " << local_id;
      break;
    case FILE_ERROR_ABORT:
      // Ignore it because this is caused by user's cancel operations.
      break;
    case FILE_ERROR_NO_CONNECTION:
      // Run the task again so that we'll retry once the connection is back.
      it->second.should_run_again = true;
      it->second.context = ClientContext(BACKGROUND);
      break;
    case FILE_ERROR_SERVICE_UNAVAILABLE:
      // Run the task again so that we'll retry once the service is back.
      it->second.should_run_again = true;
      it->second.context = ClientContext(BACKGROUND);
      retry_delay = long_delay_;
      operation_delegate_->OnDriveSyncError(
          file_system::DRIVE_SYNC_ERROR_SERVICE_UNAVAILABLE, local_id);
      break;
    case FILE_ERROR_NO_SERVER_SPACE:
      operation_delegate_->OnDriveSyncError(
          file_system::DRIVE_SYNC_ERROR_NO_SERVER_SPACE, local_id);
      break;
    default:
      operation_delegate_->OnDriveSyncError(
          file_system::DRIVE_SYNC_ERROR_MISC, local_id);
      LOG(WARNING) << "Failed: type = " << type << ", id = " << local_id
                   << ": " << FileErrorToString(error);
  }

  for (size_t i = 0; i < it->second.waiting_callbacks.size(); ++i) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(it->second.waiting_callbacks[i], error));
  }
  it->second.waiting_callbacks.clear();

  if (it->second.should_run_again) {
    DVLOG(1) << "Running again: type = " << type << ", id = " << local_id;
    it->second.state = PENDING;
    it->second.should_run_again = false;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SyncClient::StartTask, weak_ptr_factory_.GetWeakPtr(), key),
        retry_delay);
  } else {
    for (size_t i = 0; i < it->second.dependent_tasks.size(); ++i)
      StartTask(it->second.dependent_tasks[i]);
    tasks_.erase(it);
  }
}

void SyncClient::OnFetchFileComplete(const std::string& local_id,
                                     FileError error,
                                     const base::FilePath& local_path,
                                     std::unique_ptr<ResourceEntry> entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  OnTaskComplete(FETCH, local_id, error);
  if (error == FILE_ERROR_ABORT) {
    // If user cancels download, unpin the file so that we do not sync the file
    // again.
    base::PostTaskAndReplyWithResult(
        blocking_task_runner_.get(), FROM_HERE,
        base::Bind(&FileCache::Unpin, base::Unretained(cache_), local_id),
        base::DoNothing::Repeatedly<FileError>());
  }
}

}  // namespace internal
}  // namespace drive
