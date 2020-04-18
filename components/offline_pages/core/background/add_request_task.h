// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_REQUEST_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_REQUEST_TASK_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/background/request_queue_store.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class AddRequestTask : public Task {
 public:
  AddRequestTask(RequestQueueStore* store,
                 const SavePageRequest& request,
                 RequestQueueStore::AddCallback callback);
  ~AddRequestTask() override;

  // Task implementation:
  void Run() override;

 private:
  // Step 1: Add the request to the store.
  void AddRequest();
  // Step 2: Calls the callback with result, completes the task.
  void CompleteWithResult(ItemActionStatus status);

  // Store from which requests will be read.
  RequestQueueStore* store_;
  // Request to be added.
  SavePageRequest request_;
  // Callback used to return the read results.
  RequestQueueStore::AddCallback callback_;

  base::WeakPtrFactory<AddRequestTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(AddRequestTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_REQUEST_TASK_H_
