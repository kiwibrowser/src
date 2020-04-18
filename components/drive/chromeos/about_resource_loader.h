// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_LOADER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "google_apis/drive/drive_common_callbacks.h"

namespace drive {

class JobScheduler;

namespace internal {

// This class is responsible to load AboutResource from the server and cache it.
class AboutResourceLoader {
 public:
  explicit AboutResourceLoader(JobScheduler* scheduler);
  ~AboutResourceLoader();

  // Returns the cached about resource.
  // NULL is returned if the cache is not available.
  const google_apis::AboutResource* cached_about_resource() const {
    return cached_about_resource_.get();
  }

  // Gets the 'latest' about resource and asynchronously runs |callback|. I.e.,
  // 1) If the last call to UpdateAboutResource call is in-flight, wait for it.
  // 2) Otherwise, if the resource is cached, just returns the cached value.
  // 3) If neither of the above hold, queries the API server by calling
  //   |UpdateAboutResource|.
  void GetAboutResource(const google_apis::AboutResourceCallback& callback);

  // Gets the about resource from the server, and caches it if successful. This
  // function calls JobScheduler::GetAboutResource internally. The cache will be
  // used in |GetAboutResource|.
  void UpdateAboutResource(const google_apis::AboutResourceCallback& callback);

 private:
  // Part of UpdateAboutResource().
  // This function should be called when the latest about resource is being
  // fetched from the server. The retrieved about resource is cloned, and one is
  // cached and the other is passed to callbacks associated with |task_id|.
  void UpdateAboutResourceAfterGetAbout(
      int task_id,
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::AboutResource> about_resource);

  JobScheduler* scheduler_;
  std::unique_ptr<google_apis::AboutResource> cached_about_resource_;

  // Identifier to denote the latest UpdateAboutResource call.
  int current_update_task_id_;
  // Mapping from each UpdateAboutResource task ID to the corresponding
  // callbacks. Note that there will be multiple callbacks for a single task
  // when GetAboutResource is called before the task completes.
  std::map<int, std::vector<google_apis::AboutResourceCallback>>
      pending_callbacks_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<AboutResourceLoader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(AboutResourceLoader);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_LOADER_H_
