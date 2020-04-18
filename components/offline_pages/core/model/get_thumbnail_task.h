// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_THUMBNAIL_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_THUMBNAIL_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_thumbnail.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class OfflinePageMetadataStoreSQL;

// GetThumbnailTask reads a thumbnail from the page_thumbnails table.
class GetThumbnailTask : public Task {
 public:
  typedef base::OnceCallback<void(std::unique_ptr<OfflinePageThumbnail>)>
      CompleteCallback;

  GetThumbnailTask(OfflinePageMetadataStoreSQL* store,
                   int64_t offline_id,
                   CompleteCallback complete_callback);
  ~GetThumbnailTask() override;

  // Task implementation:
  void Run() override;

 private:
  typedef std::unique_ptr<OfflinePageThumbnail> Result;

  void Complete(Result result);

  OfflinePageMetadataStoreSQL* store_;
  int64_t offline_id_;
  base::OnceCallback<void(std::unique_ptr<OfflinePageThumbnail>)>
      complete_callback_;
  base::WeakPtrFactory<GetThumbnailTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(GetThumbnailTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_THUMBNAIL_TASK_H_
