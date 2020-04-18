// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_UPDATE_FILE_PATH_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_UPDATE_FILE_PATH_TASK_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/model/get_pages_task.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

using ReadResult = GetPagesTask::ReadResult;

class OfflinePageMetadataStoreSQL;

// Task that updates the file path in the metadata store. It takes the offline
// ID of the page accessed, the new file path, and the completion callback.
class UpdateFilePathTask : public Task {
 public:
  UpdateFilePathTask(OfflinePageMetadataStoreSQL* store,
                     int64_t offline_id,
                     const base::FilePath& file_path,
                     UpdateFilePathDoneCallback callback);
  ~UpdateFilePathTask() override;

  // Task implementation.
  void Run() override;

 private:
  void OnUpdateFilePathDone(bool result);

  // The metadata store used to update the page. Not owned.
  OfflinePageMetadataStoreSQL* store_;

  int64_t offline_id_;
  base::FilePath file_path_;
  UpdateFilePathDoneCallback callback_;

  base::WeakPtrFactory<UpdateFilePathTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(UpdateFilePathTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_UPDATE_FILE_PATH_TASK_H_
