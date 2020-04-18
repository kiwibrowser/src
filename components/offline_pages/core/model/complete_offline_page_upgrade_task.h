// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_COMPLETE_OFFLINE_PAGE_UPRGRADE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_COMPLETE_OFFLINE_PAGE_UPRGRADE_TASK_H_

#include <stdint.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/model/offline_page_upgrade_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

// This task is responsible for completing the upgrade process for an offline
// page.
class CompleteOfflinePageUpgradeTask : public Task {
 public:
  CompleteOfflinePageUpgradeTask(OfflinePageMetadataStoreSQL* store,
                                 int64_t offline_id,
                                 const base::FilePath& temporary_file_path,
                                 const base::FilePath& target_file_path,
                                 const std::string& digest,
                                 int64_t file_size,
                                 CompleteUpgradeCallback callback);
  ~CompleteOfflinePageUpgradeTask() override;

  // Task implementation.
  void Run() override;

 private:
  void InformUpgradeAttemptDone(CompleteUpgradeStatus result);

  // The store containing the pages to be cleared. Not owned.
  OfflinePageMetadataStoreSQL* store_;

  // ID of the item that needs to be updated.
  int64_t offline_id_;

  // Name of the temporary file. This file is already upgraded and needs to
  // replace the old file in the DB.
  base::FilePath temporary_file_path_;

  // Final name that the archive should have in the public location.
  base::FilePath target_file_path_;

  // Digest of the temporary file.
  std::string digest_;

  // Expected file size of the temporary file, which is double checked in
  // archive verification step.
  int64_t file_size_;

  // Callback to return the result of starting the upgrade process.
  CompleteUpgradeCallback callback_;

  base::WeakPtrFactory<CompleteOfflinePageUpgradeTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CompleteOfflinePageUpgradeTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_COMPLETE_OFFLINE_PAGE_UPRGRADE_TASK_H_
