// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_TEAM_DRIVE_LIST_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_TEAM_DRIVE_LIST_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_requests.h"

namespace base {
class CancellationFlag;
class SequencedTaskRunner;
}  // namespace base

namespace drive {

class EventLogger;
class JobScheduler;

namespace internal {

class ChangeList;
class LoaderController;

// Loads from the server the complete list of the users team drives. This class
// is used to obtain the initial list of team drives that a user has access to,
// so that team drive specific change lists and XMPP notifications can be
// accessed.
// Upon loading the resource metadata will be updated to reflect the team drives
// that the user has access to.
class TeamDriveListLoader {
 public:
  TeamDriveListLoader(EventLogger* logger,
                      base::SequencedTaskRunner* blocking_task_runner,
                      ResourceMetadata* resource_metadata,
                      JobScheduler* scheduler,
                      LoaderController* apply_task_controller);

  ~TeamDriveListLoader();

  // Indicates whether there is a request for the team drives list in progress.
  bool IsRefreshing() const;

  // Gets the team drive list for the user from the server. |callback| must not
  // be null.
  void CheckForUpdates(const FileOperationCallback& callback);

  // Loads the state of the users team drives. |callback| must not be null.
  void LoadIfNeeded(const FileOperationCallback& callback);

 private:
  void OnTeamDriveListLoaded(
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::TeamDriveList> team_drives);

  void OnTeamDriveListLoadComplete(FileError error);

  EventLogger* logger_;  // Not owned.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  std::unique_ptr<base::CancellationFlag> in_shutdown_;
  std::vector<std::unique_ptr<ChangeList>> change_lists_;
  std::vector<FileOperationCallback> pending_load_callbacks_;
  bool loaded_ = false;

  ResourceMetadata* resource_metadata_;  // Not owned.
  JobScheduler* scheduler_;              // Not owned.
  LoaderController* loader_controller_;  // Not owned.

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<TeamDriveListLoader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(TeamDriveListLoader);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_TEAM_DRIVE_LIST_LOADER_H_
