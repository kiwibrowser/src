// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_IN_MEMORY_STORE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_IN_MEMORY_STORE_H_

#include <stdint.h>

#include <map>

#include "base/macros.h"
#include "components/offline_pages/core/background/request_queue_store.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

// Interface for classes storing save page requests.
class RequestQueueInMemoryStore : public RequestQueueStore {
 public:
  enum class TestScenario {
    SUCCESSFUL,
    LOAD_FAILED_RESET_SUCCESS,
    LOAD_FAILED_RESET_FAILED,
  };

  RequestQueueInMemoryStore();
  explicit RequestQueueInMemoryStore(TestScenario scenario);
  ~RequestQueueInMemoryStore() override;

  // RequestQueueStore implementaiton.
  void Initialize(InitializeCallback callback) override;
  void GetRequests(GetRequestsCallback callback) override;
  void GetRequestsByIds(const std::vector<int64_t>& request_ids,
                        UpdateCallback callback) override;
  void AddRequest(const SavePageRequest& offline_page,
                  AddCallback callback) override;
  void UpdateRequests(const std::vector<SavePageRequest>& requests,
                      UpdateCallback callback) override;
  void RemoveRequests(const std::vector<int64_t>& request_ids,
                      UpdateCallback callback) override;
  void Reset(ResetCallback callback) override;
  StoreState state() const override;

 private:
  typedef std::map<int64_t, SavePageRequest> RequestsMap;
  RequestsMap requests_;
  StoreState state_;
  TestScenario scenario_;

  DISALLOW_COPY_AND_ASSIGN(RequestQueueInMemoryStore);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_IN_MEMORY_STORE_H_
