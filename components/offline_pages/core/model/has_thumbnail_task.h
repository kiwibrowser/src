// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_HAS_THUMBNAIL_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_HAS_THUMBNAIL_TASK_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class OfflinePageMetadataStoreSQL;

// Checks if a thumbnail exists for the specified offline id.
class HasThumbnailTask : public Task {
 public:
  using ThumbnailExistsCallback = base::OnceCallback<void(bool)>;

  HasThumbnailTask(OfflinePageMetadataStoreSQL* store,
                   int64_t offline_id,
                   ThumbnailExistsCallback exists_callback);
  ~HasThumbnailTask() override;

  // Task implementation:
  void Run() override;

 private:
  void OnThumbnailExists(bool exists);

  OfflinePageMetadataStoreSQL* store_;
  int64_t offline_id_;
  ThumbnailExistsCallback exists_callback_;
  base::WeakPtrFactory<HasThumbnailTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(HasThumbnailTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_HAS_THUMBNAIL_TASK_H_
