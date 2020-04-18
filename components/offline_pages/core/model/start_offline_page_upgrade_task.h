// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_START_OFFLINE_PAGE_UPRGRADE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_START_OFFLINE_PAGE_UPRGRADE_TASK_H_

#include <stdint.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/model/offline_page_upgrade_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

// This task is responsible for starting the upgrade process for an offline
// page.
class StartOfflinePageUpgradeTask : public Task {
 public:
  StartOfflinePageUpgradeTask(OfflinePageMetadataStoreSQL* store,
                              int64_t offline_id,
                              const base::FilePath& target_directory,
                              StartUpgradeCallback callback);
  ~StartOfflinePageUpgradeTask() override;

  // Task implementation.
  void Run() override;

 private:
  void InformUpgradeAttemptDone(StartUpgradeResult result);

  // The store containing the pages to be cleared. Not owned.
  OfflinePageMetadataStoreSQL* store_;

  // ID of the item that needs to be updated.
  int64_t offline_id_;

  // Directory where the file is expected after upgrade.
  base::FilePath target_directory_;

  // Callback to return the result of starting the upgrade process.
  StartUpgradeCallback callback_;

  base::WeakPtrFactory<StartOfflinePageUpgradeTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StartOfflinePageUpgradeTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_START_OFFLINE_PAGE_UPRGRADE_TASK_H_
