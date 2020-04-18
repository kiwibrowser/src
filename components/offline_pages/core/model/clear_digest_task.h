// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_DIGEST_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_DIGEST_TASK_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

// Task that clears the digest field of a page in the metadata store. It takes
// the offline ID of the page that needs to have digest cleared.
// There is no callback needed for this task.
class ClearDigestTask : public Task {
 public:
  ClearDigestTask(OfflinePageMetadataStoreSQL* store, int64_t offline_id);
  ~ClearDigestTask() override;

  // Task implementation.
  void Run() override;

 private:
  void OnClearDigestDone(bool result);

  // The metadata store used to clear the digest. Not owned.
  OfflinePageMetadataStoreSQL* store_;

  int64_t offline_id_;

  base::WeakPtrFactory<ClearDigestTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ClearDigestTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_DIGEST_TASK_H_
