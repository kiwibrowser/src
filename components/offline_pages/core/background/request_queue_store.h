// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_H_

#include <stdint.h>
#include <vector>

#include "base/callback.h"
#include "components/offline_pages/core/background/request_queue.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/offline_store_types.h"

namespace offline_pages {

// Interface for classes storing save page requests.
class RequestQueueStore {
 public:
  enum class UpdateStatus {
    ADDED,    // Request was added successfully.
    UPDATED,  // Request was updated successfully.
    FAILED,   // Add or update attempt failed.
  };

  using UpdateCallback = RequestQueue::UpdateCallback;

  typedef base::OnceCallback<void(bool /* success */)> InitializeCallback;
  typedef base::OnceCallback<void(bool /* success */)> ResetCallback;
  typedef base::OnceCallback<void(
      bool /* success */,
      std::vector<std::unique_ptr<SavePageRequest>> /* requests */)>
      GetRequestsCallback;
  typedef base::OnceCallback<void(ItemActionStatus)> AddCallback;

  virtual ~RequestQueueStore(){};

  // Initializes the store. Should be called before any other methods.
  virtual void Initialize(InitializeCallback callback) = 0;

  // Gets all of the requests from the store.
  virtual void GetRequests(GetRequestsCallback callback) = 0;

  // Gets requests with specified IDs from the store. UpdateCallback is used
  // instead of GetRequestsCallback to indicate which requests where not found.
  virtual void GetRequestsByIds(const std::vector<int64_t>& request_ids,
                                UpdateCallback callback) = 0;

  // Asynchronously adds request in store. Fails if request with the same
  // offline ID already exists.
  virtual void AddRequest(const SavePageRequest& offline_page,
                          AddCallback callback) = 0;

  // Asynchronously updates requests in store.
  virtual void UpdateRequests(const std::vector<SavePageRequest>& requests,
                              UpdateCallback callback) = 0;

  // Asynchronously removes requests from the store using their IDs.
  // Result of the update, and a number of removed pages is passed in the
  // callback.
  // Result of remove should be false, when one of the provided items couldn't
  // be deleted, e.g. because it was missing.
  virtual void RemoveRequests(const std::vector<int64_t>& request_ids,
                              UpdateCallback callback) = 0;

  // Resets the store (removes any existing data).
  virtual void Reset(ResetCallback callback) = 0;

  // Gets the store state.
  virtual StoreState state() const = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_H_
