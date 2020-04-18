// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_SYNC_CLIENT_H_
#define COMPONENTS_DRIVE_CHROMEOS_SYNC_CLIENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/file_errors.h"
#include "components/drive/job_scheduler.h"

namespace base {
class SequencedTaskRunner;
}

namespace drive {

class JobScheduler;
class ResourceEntry;
struct ClientContext;

namespace file_system {
class DownloadOperation;
class OperationDelegate;
}

namespace internal {

class EntryUpdatePerformer;
class FileCache;
class LoaderController;
class ResourceMetadata;

// The SyncClient is used to synchronize pinned files on Drive and the
// cache on the local drive.
//
// If the user logs out before fetching of the pinned files is complete, this
// client resumes fetching operations next time the user logs in, based on
// the states left in the cache.
class SyncClient {
 public:
  SyncClient(base::SequencedTaskRunner* blocking_task_runner,
             file_system::OperationDelegate* delegate,
             JobScheduler* scheduler,
             ResourceMetadata* metadata,
             FileCache* cache,
             LoaderController* loader_controller,
             const base::FilePath& temporary_file_directory);
  virtual ~SyncClient();

  // Adds a fetch task.
  void AddFetchTask(const std::string& local_id);

  // Removes a fetch task.
  void RemoveFetchTask(const std::string& local_id);

  // Adds a update task.
  void AddUpdateTask(const ClientContext& context, const std::string& local_id);

  // Waits for the update task to complete and runs the callback.
  // Returns false if no task is found for the spcecified ID.
  bool WaitForUpdateTaskToComplete(const std::string& local_id,
                                   const FileOperationCallback& callback);

  // Starts processing the backlog (i.e. pinned-but-not-filed files and
  // dirty-but-not-uploaded files). Kicks off retrieval of the local
  // IDs of these files, and then starts the sync loop.
  void StartProcessingBacklog();

  // Starts checking the existing pinned files to see if these are
  // up to date. If stale files are detected, the local IDs of these files
  // are added and the sync loop is started.
  void StartCheckingExistingPinnedFiles();

  // Sets a delay for testing.
  void set_delay_for_testing(const base::TimeDelta& delay) {
    delay_ = delay;
  }

 private:
  // Types of sync tasks.
  enum SyncType {
    FETCH,  // Fetch a file from the Drive server.
    UPDATE,  // Updates an entry's metadata or content on the Drive server.
  };

  // States of sync tasks.
  enum SyncState {
    SUSPENDED,  // Task is currently inactive.
    PENDING,  // Task is going to run.
    RUNNING,  // Task is running.
  };

  typedef std::pair<SyncType, std::string> SyncTaskKey;

  struct SyncTask {
    SyncTask();
    SyncTask(const SyncTask& other);
    ~SyncTask();
    SyncState state;
    ClientContext context;
    base::Callback<base::Closure(const ClientContext& context)> task;
    bool should_run_again;
    base::Closure cancel_closure;
    std::vector<SyncTaskKey> dependent_tasks;
    std::vector<FileOperationCallback> waiting_callbacks;
  };

  typedef std::map<SyncTaskKey, SyncTask> SyncTasks;

  // Performs a FETCH task.
  base::Closure PerformFetchTask(const std::string& local_id,
                                 const ClientContext& context);

  // Adds a FETCH task.
  void AddFetchTaskInternal(const std::string& local_id,
                            const base::TimeDelta& delay);

  // Performs a UPDATE task.
  base::Closure PerformUpdateTask(const std::string& local_id,
                                  const ClientContext& context);

  // Adds a UPDATE task.
  void AddUpdateTaskInternal(const ClientContext& context,
                             const std::string& local_id,
                             const base::TimeDelta& delay);

  // Adds the given task. If the same task is found, does nothing.
  void AddTask(const SyncTasks::key_type& key,
               const SyncTask& task,
               const base::TimeDelta& delay);

  // Called when a task is ready to start.
  void StartTask(const SyncTasks::key_type& key);
  void StartTaskAfterGetParentResourceEntry(const SyncTasks::key_type& key,
                                            const ResourceEntry* parent,
                                            FileError error);

  // Called when the local IDs of files in the backlog are obtained.
  void OnGetLocalIdsOfBacklog(const std::vector<std::string>* to_fetch,
                              const std::vector<std::string>* to_update);

  // Adds fetch tasks.
  void AddFetchTasks(const std::vector<std::string>* local_ids);

  // Called when a task is completed.
  void OnTaskComplete(SyncType type,
                      const std::string& local_id,
                      FileError error);

  // Called when the file for |local_id| is fetched.
  void OnFetchFileComplete(const std::string& local_id,
                           FileError error,
                           const base::FilePath& local_path,
                           std::unique_ptr<ResourceEntry> entry);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  file_system::OperationDelegate* operation_delegate_;
  ResourceMetadata* metadata_;
  FileCache* cache_;

  // Used to fetch pinned files.
  std::unique_ptr<file_system::DownloadOperation> download_operation_;

  // Used to update entry metadata.
  std::unique_ptr<EntryUpdatePerformer> entry_update_performer_;

  // Sync tasks to be processed.
  SyncTasks tasks_;

  // The delay is used for delaying processing tasks in AddTask().
  base::TimeDelta delay_;

  // The delay is used for delaying retry of tasks on server errors.
  base::TimeDelta long_delay_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SyncClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncClient);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_SYNC_CLIENT_H_
