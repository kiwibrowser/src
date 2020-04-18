// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_CHILD_PROCESS_WATCHER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_CHILD_PROCESS_WATCHER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/common/service_manager_connection.h"

namespace resource_coordinator {

class ProcessResourceCoordinator;

class BrowserChildProcessWatcher : public content::BrowserChildProcessObserver {
 public:
  BrowserChildProcessWatcher();
  ~BrowserChildProcessWatcher() override;

 private:
  // BrowserChildProcessObserver overrides.
  void BrowserChildProcessLaunchedAndConnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessHostDisconnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessCrashed(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;
  void BrowserChildProcessKilled(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;

  void GPUProcessStopped();

  std::unique_ptr<resource_coordinator::ProcessResourceCoordinator>
      gpu_process_resource_coordinator_;

  DISALLOW_COPY_AND_ASSIGN(BrowserChildProcessWatcher);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_CHILD_PROCESS_WATCHER_H_
