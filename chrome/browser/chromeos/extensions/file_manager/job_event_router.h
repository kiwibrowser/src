// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_JOB_EVENT_ROUTER_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_JOB_EVENT_ROUTER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/common/extensions/api/file_manager_private.h"
#include "components/drive/job_list.h"
#include "extensions/browser/extension_event_histogram_value.h"
#include "url/gurl.h"

namespace file_manager {

// Files app's event router handling job related events.
class JobEventRouter : public drive::JobListObserver {
 public:
  explicit JobEventRouter(const base::TimeDelta& event_delay);
  ~JobEventRouter() override;

  // drive::JobListObserver overrides.
  void OnJobAdded(const drive::JobInfo& job_info) override;
  void OnJobUpdated(const drive::JobInfo& job_info) override;
  void OnJobDone(const drive::JobInfo& job_info,
                 drive::FileError error) override;

 protected:
  // Helper method for getting set of listener extension ids.
  virtual std::set<std::string>
  GetFileTransfersUpdateEventListenerExtensionIds() = 0;

  // Helper method for converting drive path to file system url.
  virtual GURL ConvertDrivePathToFileSystemUrl(
      const base::FilePath& file_path,
      const std::string& extension_id) = 0;

  // Helper method for dispatching an event to an extension.
  virtual void DispatchEventToExtension(
      const std::string& extension_id,
      extensions::events::HistogramValue histogram_value,
      const std::string& event_name,
      std::unique_ptr<base::ListValue> event_args) = 0;

 private:
  // Request sending transfer event with |job_info| and |state|.
  // If |immediate| is true, the event will be dispatched synchronously.
  // Otherwise, the event is throttled, or may be skipped.
  void ScheduleDriveFileTransferEvent(
      const drive::JobInfo& job_info,
      extensions::api::file_manager_private::TransferState state,
      bool immediate);

  // Send transfer event requested by ScheduleDriveFileTransferEvent at last.
  void SendDriveFileTransferEvent();

  // Update |num_completed_bytes_| and |num_total_bytes_| depends on |job|.
  void UpdateBytes(const drive::JobInfo& job_info);

  // Dispatches FileTransfersUpdate event to an extension.
  void DispatchFileTransfersUpdateEventToExtension(
      const std::string& extension_id,
      const drive::JobInfo& job_info,
      const extensions::api::file_manager_private::TransferState& state,
      const int64_t num_total_jobs,
      const int64_t num_completed_bytes,
      const int64_t num_total_bytes);

  // Delay time before sending progress events.
  base::TimeDelta event_delay_;

  // Set of job that are in the job schedular.
  std::map<drive::JobID, std::unique_ptr<drive::JobInfo>> drive_jobs_;

  // Job info of pending event. |ScheduleDriveFileTransferEvent| registers
  // timeout callback to dispatch this.
  std::unique_ptr<drive::JobInfo> pending_job_info_;

  // Transfer state of pending event.
  extensions::api::file_manager_private::TransferState pending_state_;

  // Computed bytes of tasks that have been processed. Once it completes all
  // tasks, it clears the variable.
  int64_t num_completed_bytes_;

  // Total bytes of tasks that have been processed. Once it completes all tasks,
  // it clears the variable.
  int64_t num_total_bytes_;

  // Thread checker.
  base::ThreadChecker thread_checker_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<JobEventRouter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(JobEventRouter);
};

}  // namespace file_manager

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_JOB_EVENT_ROUTER_H_
