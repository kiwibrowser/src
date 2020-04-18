// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/request_queue_in_memory_store.h"

#include <unordered_set>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

RequestQueueInMemoryStore::RequestQueueInMemoryStore()
    : state_(StoreState::NOT_LOADED), scenario_(TestScenario::SUCCESSFUL) {}

RequestQueueInMemoryStore::RequestQueueInMemoryStore(TestScenario scenario)
    : state_(StoreState::NOT_LOADED), scenario_(scenario) {}

RequestQueueInMemoryStore::~RequestQueueInMemoryStore() {}

void RequestQueueInMemoryStore::Initialize(InitializeCallback callback) {
  if (scenario_ == TestScenario::SUCCESSFUL)
    state_ = StoreState::LOADED;
  else
    state_ = StoreState::FAILED_LOADING;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), state_ == StoreState::LOADED));
}

void RequestQueueInMemoryStore::GetRequests(GetRequestsCallback callback) {
  DCHECK_NE(state_, StoreState::NOT_LOADED);
  std::vector<std::unique_ptr<SavePageRequest>> result_requests;
  for (const auto& id_request_pair : requests_) {
    std::unique_ptr<SavePageRequest> request(
        new SavePageRequest(id_request_pair.second));
    result_requests.push_back(std::move(request));
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), true, std::move(result_requests)));
}

void RequestQueueInMemoryStore::GetRequestsByIds(
    const std::vector<int64_t>& request_ids,
    UpdateCallback callback) {
  DCHECK_NE(state_, StoreState::NOT_LOADED);
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(state()));

  ItemActionStatus status;
  // Make sure not to include the same request multiple times, while preserving
  // the order of non-duplicated IDs in the result.
  std::unordered_set<int64_t> processed_ids;
  for (const auto& request_id : request_ids) {
    if (!processed_ids.insert(request_id).second)
      continue;
    RequestsMap::iterator iter = requests_.find(request_id);
    if (iter != requests_.end()) {
      status = ItemActionStatus::SUCCESS;
      result->updated_items.push_back(iter->second);
    } else {
      status = ItemActionStatus::NOT_FOUND;
    }
    result->item_statuses.push_back(std::make_pair(request_id, status));
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(result)));
}

void RequestQueueInMemoryStore::AddRequest(const SavePageRequest& request,
                                           AddCallback callback) {
  DCHECK_NE(state_, StoreState::NOT_LOADED);
  RequestsMap::iterator iter = requests_.find(request.request_id());
  ItemActionStatus status;
  if (iter == requests_.end()) {
    requests_.insert(iter, std::make_pair(request.request_id(), request));
    status = ItemActionStatus::SUCCESS;
  } else {
    status = ItemActionStatus::ALREADY_EXISTS;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), status));
}

void RequestQueueInMemoryStore::UpdateRequests(
    const std::vector<SavePageRequest>& requests,
    UpdateCallback callback) {
  DCHECK_NE(state_, StoreState::NOT_LOADED);
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(state()));

  ItemActionStatus status;
  for (const auto& request : requests) {
    RequestsMap::iterator iter = requests_.find(request.request_id());
    if (iter != requests_.end()) {
      status = ItemActionStatus::SUCCESS;
      iter->second = request;
      result->updated_items.push_back(request);
    } else {
      status = ItemActionStatus::NOT_FOUND;
    }
    result->item_statuses.push_back(
        std::make_pair(request.request_id(), status));
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(result)));
}

void RequestQueueInMemoryStore::RemoveRequests(
    const std::vector<int64_t>& request_ids,
    UpdateCallback callback) {
  DCHECK_NE(state_, StoreState::NOT_LOADED);
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(StoreState::LOADED));

  ItemActionStatus status;
  // If we find a request, mark it as succeeded, and put it in the request list.
  // Otherwise mark it as failed.
  for (auto request_id : request_ids) {
    RequestsMap::iterator iter = requests_.find(request_id);
    if (iter != requests_.end()) {
      status = ItemActionStatus::SUCCESS;
      result->updated_items.push_back(iter->second);
      requests_.erase(iter);
    } else {
      status = ItemActionStatus::NOT_FOUND;
    }
    result->item_statuses.push_back(std::make_pair(request_id, status));
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(result)));
}

void RequestQueueInMemoryStore::Reset(ResetCallback callback) {
  if (scenario_ != TestScenario::LOAD_FAILED_RESET_FAILED) {
    requests_.clear();
    state_ = StoreState::NOT_LOADED;
    scenario_ = TestScenario::SUCCESSFUL;
  } else {
    state_ = StoreState::FAILED_RESET;
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), state_ == StoreState::NOT_LOADED));
}

StoreState RequestQueueInMemoryStore::state() const {
  return state_;
}

}  // namespace offline_pages
