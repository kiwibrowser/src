// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/add_request_task.h"

#include "base/bind.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

AddRequestTask::AddRequestTask(RequestQueueStore* store,
                               const SavePageRequest& request,
                               RequestQueueStore::AddCallback callback)
    : store_(store),
      request_(request),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {}

AddRequestTask::~AddRequestTask() {}

void AddRequestTask::Run() {
  AddRequest();
}

void AddRequestTask::AddRequest() {
  store_->AddRequest(request_,
                     base::BindOnce(&AddRequestTask::CompleteWithResult,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void AddRequestTask::CompleteWithResult(ItemActionStatus status) {
  std::move(callback_).Run(status);
  TaskComplete();
}

}  // namespace offline_pages
