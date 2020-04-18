// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STARTUP_MAINTENANCE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STARTUP_MAINTENANCE_TASK_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class ArchiveManager;
class ClientPolicyController;
class OfflinePageMetadataStoreSQL;

// This task is responsible for executing maintenance sub-tasks during Chrome
// startup, including: temporary page consistency check, legacy directory
// cleaning and report storage usage UMA.
class StartupMaintenanceTask : public Task {
 public:
  StartupMaintenanceTask(OfflinePageMetadataStoreSQL* store,
                         ArchiveManager* archive_manager,
                         ClientPolicyController* policy_controller);
  ~StartupMaintenanceTask() override;

  // Task implementation:
  void Run() override;

 private:
  void OnStartupMaintenanceDone(bool result);

  // The store containing the offline pages. Not owned.
  OfflinePageMetadataStoreSQL* store_;
  // The archive manager storing archive directories. Not owned.
  ArchiveManager* archive_manager_;
  // The policy controller which is used to acquire names of namespaces. Not
  // owned.
  ClientPolicyController* policy_controller_;

  base::WeakPtrFactory<StartupMaintenanceTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StartupMaintenanceTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STARTUP_MAINTENANCE_TASK_H_
