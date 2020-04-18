// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STORE_THUMBNAIL_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STORE_THUMBNAIL_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_thumbnail.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class OfflinePageMetadataStoreSQL;

// StoreThumbnailTask stores a thumbnail in the page_thumbnails table.
class StoreThumbnailTask : public Task {
 public:
  typedef base::OnceCallback<void(bool)> CompleteCallback;

  StoreThumbnailTask(OfflinePageMetadataStoreSQL* store,
                     OfflinePageThumbnail thumbnail,
                     CompleteCallback complete_callback);
  ~StoreThumbnailTask() override;

  // Task implementation:
  void Run() override;

 private:
  void Complete(bool success);

  OfflinePageMetadataStoreSQL* store_;
  OfflinePageThumbnail thumbnail_;
  CompleteCallback complete_callback_;
  base::WeakPtrFactory<StoreThumbnailTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StoreThumbnailTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_STORE_THUMBNAIL_TASK_H_
