// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_JOB_SCHEDULER_H_
#define COMPONENTS_DRIVE_JOB_SCHEDULER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/drive/drive_uploader.h"
#include "components/drive/job_list.h"
#include "components/drive/job_queue.h"
#include "components/drive/service/drive_service_interface.h"
#include "net/base/network_change_notifier.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"

class PrefService;

namespace drive {

class EventLogger;

// Priority of a job.  Higher values are lower priority.
enum ContextType {
  USER_INITIATED,
  BACKGROUND,
  // Indicates the number of values of this enum.
  NUM_CONTEXT_TYPES,
};

struct ClientContext {
  explicit ClientContext(ContextType in_type) : type(in_type) {}
  ContextType type;
};

// The JobScheduler is responsible for queuing and scheduling drive jobs.
// Because jobs are executed concurrently by priority and retried for network
// failures, there is no guarantee of orderings.
//
// Jobs are grouped into two priority levels:
//   - USER_INITIATED jobs are those occur as a result of direct user actions.
//   - BACKGROUND jobs runs in response to state changes, server actions, etc.
// USER_INITIATED jobs must be handled immediately, thus have higher priority.
// BACKGROUND jobs run only after all USER_INITIATED jobs have run.
//
// Orthogonally, jobs are grouped into two types:
//   - "File jobs" transfer the contents of files.
//   - "Metadata jobs" operates on file metadata or the directory structure.
// On WiFi or Ethernet connections, all types of jobs just run.
// On mobile connections (2G/3G/4G), we don't want large background traffic.
// USER_INITIATED jobs or metadata jobs will run. BACKGROUND file jobs wait
// in the queue until the network type changes.
// On offline case, no jobs run. USER_INITIATED jobs fail immediately.
// BACKGROUND jobs stay in the queue and wait for network connection.
class JobScheduler : public net::NetworkChangeNotifier::NetworkChangeObserver,
                     public JobListInterface {
 public:
  JobScheduler(PrefService* pref_service,
               EventLogger* logger,
               DriveServiceInterface* drive_service,
               base::SequencedTaskRunner* blocking_task_runner,
               device::mojom::WakeLockProviderPtr wake_lock_provider);

  ~JobScheduler() override;

  // JobListInterface overrides.
  std::vector<JobInfo> GetJobInfoList() override;
  void AddObserver(JobListObserver* observer) override;
  void RemoveObserver(JobListObserver* observer) override;
  void CancelJob(JobID job_id) override;
  void CancelAllJobs() override;

  // Adds a GetAppList operation to the queue.
  // |callback| must not be null.
  void GetAppList(const google_apis::AppListCallback& callback);

  // Adds a GetAboutResource operation to the queue.
  // |callback| must not be null.
  void GetAboutResource(const google_apis::AboutResourceCallback& callback);

  // Adds a GetStartPageToken operation to the queue.
  // If |team_drive_id| is empty then it will return the start token for the
  // users changelog.
  // |callback| must not be null.
  void GetStartPageToken(const std::string& team_drive_id,
                         const google_apis::StartPageTokenCallback& callback);

  // Adds a GetAllTeamDriveList operation to the queue.
  // |callback| must not be null.
  void GetAllTeamDriveList(const google_apis::TeamDriveListCallback& callback);

  // Adds a GetAllFileList operation to the queue.
  // |callback| must not be null.
  void GetAllFileList(const google_apis::FileListCallback& callback);

  // Adds a GetFileListInDirectory operation to the queue.
  // |callback| must not be null.
  void GetFileListInDirectory(const std::string& directory_resource_id,
                              const google_apis::FileListCallback& callback);

  // Adds a Search operation to the queue.
  // |callback| must not be null.
  void Search(const std::string& search_query,
              const google_apis::FileListCallback& callback);

  // Adds a GetChangeList operation to the queue.
  // |callback| must not be null.
  void GetChangeList(int64_t start_changestamp,
                     const google_apis::ChangeListCallback& callback);

  // Adds a GetChangeList operation to the queue, where |start_page_token|
  // is used to specify where to start retrieving the change list from.
  // If |team_drive_id| is empty then it will return the change list for the
  // users changelog.
  // |callback| must not be null.
  void GetChangeList(const std::string& team_drive_id,
                     const std::string& start_page_token,
                     const google_apis::ChangeListCallback& callback);

  // Adds GetRemainingChangeList operation to the queue.
  // |callback| must not be null.
  void GetRemainingChangeList(const GURL& next_link,
                              const google_apis::ChangeListCallback& callback);

  // Adds GetRemainingTeamDriveList operation to the queue.
  // |callback| must not be null.
  void GetRemainingTeamDriveList(
      const std::string& page_token,
      const google_apis::TeamDriveListCallback& callback);

  // Adds GetRemainingFileList operation to the queue.
  // |callback| must not be null.
  void GetRemainingFileList(const GURL& next_link,
                            const google_apis::FileListCallback& callback);

  // Adds a GetFileResource operation to the queue.
  void GetFileResource(const std::string& resource_id,
                       const ClientContext& context,
                       const google_apis::FileResourceCallback& callback);

  // Adds a GetShareUrl operation to the queue.
  void GetShareUrl(const std::string& resource_id,
                   const GURL& embed_origin,
                   const ClientContext& context,
                   const google_apis::GetShareUrlCallback& callback);

  // Adds a TrashResource operation to the queue.
  void TrashResource(const std::string& resource_id,
                     const ClientContext& context,
                     const google_apis::EntryActionCallback& callback);

  // Adds a CopyResource operation to the queue.
  void CopyResource(const std::string& resource_id,
                    const std::string& parent_resource_id,
                    const std::string& new_title,
                    const base::Time& last_modified,
                    const google_apis::FileResourceCallback& callback);

  // Adds a UpdateResource operation to the queue.
  void UpdateResource(const std::string& resource_id,
                      const std::string& parent_resource_id,
                      const std::string& new_title,
                      const base::Time& last_modified,
                      const base::Time& last_viewed_by_me,
                      const google_apis::drive::Properties& properties,
                      const ClientContext& context,
                      const google_apis::FileResourceCallback& callback);

  // Adds a AddResourceToDirectory operation to the queue.
  void AddResourceToDirectory(const std::string& parent_resource_id,
                              const std::string& resource_id,
                              const google_apis::EntryActionCallback& callback);

  // Adds a RemoveResourceFromDirectory operation to the queue.
  void RemoveResourceFromDirectory(
      const std::string& parent_resource_id,
      const std::string& resource_id,
      const ClientContext& context,
      const google_apis::EntryActionCallback& callback);

  // Adds a AddNewDirectory operation to the queue.
  void AddNewDirectory(const std::string& parent_resource_id,
                       const std::string& directory_title,
                       const AddNewDirectoryOptions& options,
                       const ClientContext& context,
                       const google_apis::FileResourceCallback& callback);

  // Adds a DownloadFile operation to the queue.
  // The first two arguments |virtual_path| and |expected_file_size| are used
  // only for filling JobInfo for the operation so that observers can get the
  // detail. The actual operation never refers these values.
  JobID DownloadFile(
      const base::FilePath& virtual_path,
      int64_t expected_file_size,
      const base::FilePath& local_cache_path,
      const std::string& resource_id,
      const ClientContext& context,
      const google_apis::DownloadActionCallback& download_action_callback,
      const google_apis::GetContentCallback& get_content_callback);

  // Adds an UploadNewFile operation to the queue.
  void UploadNewFile(const std::string& parent_resource_id,
                     int64_t expected_file_size,
                     const base::FilePath& drive_file_path,
                     const base::FilePath& local_file_path,
                     const std::string& title,
                     const std::string& content_type,
                     const UploadNewFileOptions& options,
                     const ClientContext& context,
                     const google_apis::FileResourceCallback& callback);

  // Adds an UploadExistingFile operation to the queue.
  void UploadExistingFile(const std::string& resource_id,
                          int64_t expected_file_size,
                          const base::FilePath& drive_file_path,
                          const base::FilePath& local_file_path,
                          const std::string& content_type,
                          const UploadExistingFileOptions& options,
                          const ClientContext& context,
                          const google_apis::FileResourceCallback& callback);

  // Adds AddPermission operation to the queue. |callback| must not be null.
  void AddPermission(const std::string& resource_id,
                     const std::string& email,
                     google_apis::drive::PermissionRole role,
                     const google_apis::EntryActionCallback& callback);

 private:
  friend class JobSchedulerTest;

  enum QueueType {
    METADATA_QUEUE,
    FILE_QUEUE,
    NUM_QUEUES
  };

  static const int kMaxJobCount[NUM_QUEUES];

  // Represents a single entry in the job map.
  struct JobEntry {
    explicit JobEntry(JobType type);
    ~JobEntry();

    // General user-visible information on the job.
    JobInfo job_info;

    // Context of the job.
    ClientContext context;

    // The number of times the jobs is retried due to server errors.
    int retry_count;

    // The callback to start the job. Called each time it is retry.
    base::Callback<google_apis::CancelCallback()> task;

    // The callback to cancel the running job. It is returned from task.Run().
    google_apis::CancelCallback cancel_callback;

    // The callback to notify an error to the client of JobScheduler.
    // This is used to notify cancel of a job that is not running yet.
    base::Callback<void(google_apis::DriveApiErrorCode)> abort_callback;

    base::ThreadChecker thread_checker_;
  };

  // Parameters for DriveUploader::ResumeUploadFile.
  struct ResumeUploadParams;

  // Creates a new job and add it to the job map.
  JobEntry* CreateNewJob(JobType type);

  // Adds the specified job to the queue and starts the job loop for the queue
  // if needed.
  void StartJob(JobEntry* job);

  // Adds the specified job to the queue.
  void QueueJob(JobID job_id);

  // Determines the next job that should run, and starts it.
  void DoJobLoop(QueueType queue_type);

  // Returns the lowest acceptable priority level of the operations that is
  // currently allowed to start for the |queue_type|.
  int GetCurrentAcceptedPriority(QueueType queue_type);

  // Updates |wait_until_| to throttle requests.
  void UpdateWait();

  // Retries the job if needed and returns false. Otherwise returns true.
  bool OnJobDone(JobID job_id, google_apis::DriveApiErrorCode error);

  // Callback for job finishing with a FileListCallback.
  void OnGetTeamDriveListJobDone(
      JobID job_id,
      const google_apis::TeamDriveListCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::TeamDriveList> team_drive_list);

  // Callback for job finishing with a FileListCallback.
  void OnGetFileListJobDone(JobID job_id,
                            const google_apis::FileListCallback& callback,
                            google_apis::DriveApiErrorCode error,
                            std::unique_ptr<google_apis::FileList> file_list);

  // Callback for job finishing with a ChangeListCallback.
  void OnGetChangeListJobDone(
      JobID job_id,
      const google_apis::ChangeListCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::ChangeList> change_list);

  // Callback for job finishing with a FileResourceCallback.
  void OnGetFileResourceJobDone(
      JobID job_id,
      const google_apis::FileResourceCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::FileResource> entry);

  // Callback for job finishing with a AboutResourceCallback.
  void OnGetAboutResourceJobDone(
      JobID job_id,
      const google_apis::AboutResourceCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::AboutResource> about_resource);

  // Callback for job finishing with a GetStartPageTokenCallback.
  void OnGetStartPageTokenDone(
      JobID job_id,
      const google_apis::StartPageTokenCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::StartPageToken> start_page_token);

  // Callback for job finishing with a GetShareUrlCallback.
  void OnGetShareUrlJobDone(
      JobID job_id,
      const google_apis::GetShareUrlCallback& callback,
      google_apis::DriveApiErrorCode error,
      const GURL& share_url);

  // Callback for job finishing with a AppListCallback.
  void OnGetAppListJobDone(JobID job_id,
                           const google_apis::AppListCallback& callback,
                           google_apis::DriveApiErrorCode error,
                           std::unique_ptr<google_apis::AppList> app_list);

  // Callback for job finishing with a EntryActionCallback.
  void OnEntryActionJobDone(JobID job_id,
                            const google_apis::EntryActionCallback& callback,
                            google_apis::DriveApiErrorCode error);

  // Callback for job finishing with a DownloadActionCallback.
  void OnDownloadActionJobDone(
      JobID job_id,
      const google_apis::DownloadActionCallback& callback,
      google_apis::DriveApiErrorCode error,
      const base::FilePath& temp_file);

  // Callback for job finishing with a UploadCompletionCallback.
  void OnUploadCompletionJobDone(
      JobID job_id,
      const ResumeUploadParams& resume_params,
      const google_apis::FileResourceCallback& callback,
      google_apis::DriveApiErrorCode error,
      const GURL& upload_location,
      std::unique_ptr<google_apis::FileResource> entry);

  // Callback for DriveUploader::ResumeUploadFile().
  void OnResumeUploadFileDone(
      JobID job_id,
      const base::Callback<google_apis::CancelCallback()>& original_task,
      const google_apis::FileResourceCallback& callback,
      google_apis::DriveApiErrorCode error,
      const GURL& upload_location,
      std::unique_ptr<google_apis::FileResource> entry);

  // Updates the progress status of the specified job.
  void UpdateProgress(JobID job_id, int64_t progress, int64_t total);

  // net::NetworkChangeNotifier::NetworkChangeObserver override.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Get the type of queue the specified job should be put in.
  QueueType GetJobQueueType(JobType type);

  // For testing only.  Disables throttling so that testing is faster.
  void SetDisableThrottling(bool disable) { disable_throttling_ = disable; }

  // Aborts a job which is not in STATE_RUNNING.
  void AbortNotRunningJob(JobEntry* job, google_apis::DriveApiErrorCode error);

  // Notifies updates to observers.
  void NotifyJobAdded(const JobInfo& job_info);
  void NotifyJobDone(const JobInfo& job_info,
                     google_apis::DriveApiErrorCode error);
  void NotifyJobUpdated(const JobInfo& job_info);

  // Gets information of the queue of the given type as string.
  std::string GetQueueInfo(QueueType type) const;

  // Returns a string representation of QueueType.
  static std::string QueueTypeToString(QueueType type);

  // The number of times operations have failed in a row, capped at
  // kMaxThrottleCount.  This is used to calculate the delay before running the
  // next task.
  int throttle_count_;

  // Jobs should not start running until this time. Used for throttling.
  base::Time wait_until_;

  // Disables throttling for testing.
  bool disable_throttling_;

  // The queues of jobs.
  std::unique_ptr<JobQueue> queue_[NUM_QUEUES];

  // The list of queued job info indexed by job IDs.
  using JobIDMap = base::IDMap<std::unique_ptr<JobEntry>>;
  JobIDMap job_map_;

  // The list of observers for the scheduler.
  base::ObserverList<JobListObserver> observer_list_;

  EventLogger* logger_;
  DriveServiceInterface* drive_service_;
  base::SequencedTaskRunner* blocking_task_runner_;
  std::unique_ptr<DriveUploaderInterface> uploader_;

  PrefService* pref_service_;

  base::ThreadChecker thread_checker_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<JobScheduler> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(JobScheduler);
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_JOB_SCHEDULER_H_
