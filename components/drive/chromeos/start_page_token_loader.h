// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_START_PAGE_TOKEN_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_START_PAGE_TOKEN_LOADER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "google_apis/drive/drive_api_requests.h"

namespace drive {

class JobScheduler;

namespace internal {

// Loads from the server and caches the start page token for a given team drive
// id. This class is used to determine where to start retrieving changes from
// the server, and as an optimization will cache the result until an
// update is triggered using UpdateStartPageToken.
class StartPageTokenLoader {
 public:
  StartPageTokenLoader(const std::string& team_drive_id,
                       JobScheduler* scheduler);
  ~StartPageTokenLoader();

  // Returns the cached start page token. If there is no cached start page token
  // then nullptr is returned.
  const google_apis::StartPageToken* cached_start_page_token() const;

  // Gets the most recent start page token and asynchronously runs |callback|.
  // How this works is:
  // - If there is an UpdateStartPageToken in flight, wait for the result and
  //   return it.
  // - It there is NOT an UpdateStartPageToken in flight, and there is a
  //   cached start page token, then return the cached value.
  // - If neither of the above are true, then start an UpdateStartPageToken
  //   request and return the result.
  void GetStartPageToken(const google_apis::StartPageTokenCallback& callback);

  // Gets the start page token from the server, and caches it if successful.
  // This function calls JobScheduler::GetStartPageToken internally. The
  // cached result will be used by GetStartPageToken.
  void UpdateStartPageToken(
      const google_apis::StartPageTokenCallback& callback);

 private:
  // This callback is used when the result of UpdateStartPage token returns from
  // the server.
  void UpdateStartPageTokenAfterGet(
      int task_id,
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::StartPageToken> start_page_token);

  // This start page token loader is bound to a single team_drive_id, and will
  // always request the token for it. If team_drive_id_ is empty, then it
  // retrieves the start page token for the users corpus.
  const std::string team_drive_id_;
  JobScheduler* scheduler_;  // Not owned
  THREAD_CHECKER(thread_checker_);

  // We may have more than one update in flight at any time (multiple calls
  // to UpdateStartPageToken will produce this scenario). We ensure that any
  // calls to GetStartPageToken are bound to the update that was in progress
  // when the GetStartPageToken was called, using the callback map below.
  int current_update_task_id_;
  std::map<int, std::vector<google_apis::StartPageTokenCallback>>
      pending_update_callbacks_;
  std::unique_ptr<google_apis::StartPageToken> cached_start_page_token_;

  base::WeakPtrFactory<StartPageTokenLoader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StartPageTokenLoader);
};

}  // namespace internal

}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_START_PAGE_TOKEN_LOADER_H_
