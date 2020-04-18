// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEANUP_THUMBNAILS_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEANUP_THUMBNAILS_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_thumbnail.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class OfflinePageMetadataStoreSQL;

// CleanupThumbnailsTask deletes thumbnails from page_thumbnails if they
// are no longer needed.
class CleanupThumbnailsTask : public Task {
 public:
  struct Result {
    bool success = false;
    int removed_thumbnails = 0;
  };

  CleanupThumbnailsTask(OfflinePageMetadataStoreSQL* store,
                        base::Time now,
                        CleanupThumbnailsCallback complete_callback);
  ~CleanupThumbnailsTask() override;

  // Task implementation:
  void Run() override;

 private:
  void Complete(Result result);
  OfflinePageMetadataStoreSQL* store_;
  base::Time now_;

  CleanupThumbnailsCallback complete_callback_;
  base::WeakPtrFactory<CleanupThumbnailsTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CleanupThumbnailsTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEANUP_THUMBNAILS_TASK_H_
