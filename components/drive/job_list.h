// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_JOB_LIST_H_
#define COMPONENTS_DRIVE_JOB_LIST_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "components/drive/file_errors.h"

namespace drive {

// Enum representing the type of job.
enum JobType {
  TYPE_GET_ABOUT_RESOURCE,
  TYPE_GET_APP_LIST,
  TYPE_GET_ALL_TEAM_DRIVE_LIST,
  TYPE_GET_ALL_RESOURCE_LIST,
  TYPE_GET_RESOURCE_LIST_IN_DIRECTORY,
  TYPE_SEARCH,
  TYPE_GET_CHANGE_LIST,
  TYPE_GET_START_PAGE_TOKEN,
  TYPE_GET_REMAINING_CHANGE_LIST,
  TYPE_GET_REMAINING_TEAM_DRIVE_LIST,
  TYPE_GET_REMAINING_FILE_LIST,
  TYPE_GET_RESOURCE_ENTRY,
  TYPE_GET_SHARE_URL,
  TYPE_TRASH_RESOURCE,
  TYPE_COPY_RESOURCE,
  TYPE_UPDATE_RESOURCE,
  TYPE_ADD_RESOURCE_TO_DIRECTORY,
  TYPE_REMOVE_RESOURCE_FROM_DIRECTORY,
  TYPE_ADD_NEW_DIRECTORY,
  TYPE_DOWNLOAD_FILE,
  TYPE_UPLOAD_NEW_FILE,
  TYPE_UPLOAD_EXISTING_FILE,
  TYPE_ADD_PERMISSION,
};

// Returns the string representation of |type|.
std::string JobTypeToString(JobType type);

// Current state of the job.
enum JobState {
  // The job is queued, but not yet executed.
  STATE_NONE,

  // The job is in the process of being handled.
  STATE_RUNNING,

  // The job failed, but has been re-added to the queue.
  STATE_RETRY,
};

// Returns the string representation of |state|.
std::string JobStateToString(JobState state);

// Unique ID assigned to each job.
typedef int32_t JobID;

// Information about a specific job that is visible to other systems.
struct JobInfo {
  explicit JobInfo(JobType job_type);

  // Type of the job.
  JobType job_type;

  // Id of the job, which can be used to query or modify it.
  JobID job_id;

  // Current state of the operation.
  JobState state;

  // The fields below are available only for jobs with job_type:
  // TYPE_DOWNLOAD_FILE, TYPE_UPLOAD_NEW_FILE, or TYPE_UPLOAD_EXISTING_FILE.

  // Number of bytes completed.
  int64_t num_completed_bytes;

  // Total bytes of this operation.
  int64_t num_total_bytes;

  // Drive path of the file that this job acts on.
  base::FilePath file_path;

  // Time when the job is started (i.e. the request is sent to the server).
  base::Time start_time;

  // Returns the string representation of the job info.
  std::string ToString() const;
};

// Checks if |job_info| represents a job for currently active file transfer.
bool IsActiveFileTransferJobInfo(const JobInfo& job_info);

// The interface for observing JobListInterface.
// All events are notified in the UI thread.
class JobListObserver {
 public:
  // Called when a new job id added.
  virtual void OnJobAdded(const JobInfo& job_info) {}

  // Called when a job id finished.
  // |error| is FILE_ERROR_OK when the job successfully finished, and a value
  // telling the reason of failure when the jobs is failed.
  virtual void OnJobDone(const JobInfo& job_info,
                         FileError error) {}

  // Called when a job status is updated.
  virtual void OnJobUpdated(const JobInfo& job_info) {}

 protected:
  virtual ~JobListObserver() {}
};

// The interface to expose the list of issued Drive jobs.
class JobListInterface {
 public:
  virtual ~JobListInterface() {}

  // Returns the list of jobs currently managed by the scheduler.
  virtual std::vector<JobInfo> GetJobInfoList() = 0;

  // Adds an observer.
  virtual void AddObserver(JobListObserver* observer) = 0;

  // Removes an observer.
  virtual void RemoveObserver(JobListObserver* observer) = 0;

  // Cancels the job.
  virtual void CancelJob(JobID job_id) = 0;

  // Cancels all the jobs.
  virtual void CancelAllJobs() = 0;
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_JOB_LIST_H_
