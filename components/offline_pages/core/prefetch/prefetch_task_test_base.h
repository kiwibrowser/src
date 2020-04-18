// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TASK_TEST_BASE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TASK_TEST_BASE_H_

#include <memory>
#include <set>

#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/test_prefetch_network_request_factory.h"
#include "components/offline_pages/core/task_test_base.h"
#include "net/url_request/test_url_fetcher_factory.h"

namespace offline_pages {
struct PrefetchItem;
class PrefetchStore;

// Base class for testing prefetch requests with simulated responses.
class PrefetchTaskTestBase : public TaskTestBase {
 public:
  // Lists all existing prefetch item states in their natural pipeline
  // progression order.
  static constexpr std::array<PrefetchItemState, 11>
      kOrderedPrefetchItemStates = {
          {PrefetchItemState::NEW_REQUEST,
           PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE,
           PrefetchItemState::AWAITING_GCM, PrefetchItemState::RECEIVED_GCM,
           PrefetchItemState::SENT_GET_OPERATION,
           PrefetchItemState::RECEIVED_BUNDLE, PrefetchItemState::DOWNLOADING,
           PrefetchItemState::DOWNLOADED, PrefetchItemState::IMPORTING,
           PrefetchItemState::FINISHED, PrefetchItemState::ZOMBIE}};

  PrefetchTaskTestBase();
  ~PrefetchTaskTestBase() override;

  void SetUp() override;
  void TearDown() override;

  // Returns all PrefetchItemState values in a vector, filtering our the ones
  // listed in |states_to_exclude|. The returned list is based off
  // |kOrderedPrefetchItemStates| and its order of states is maintained.
  std::vector<PrefetchItemState> GetAllStatesExcept(
      std::set<PrefetchItemState> states_to_exclude);

  int64_t InsertPrefetchItemInStateWithOperation(std::string operation_name,
                                                 PrefetchItemState state);

  std::set<PrefetchItem> FilterByState(const std::set<PrefetchItem>& items,
                                       PrefetchItemState state) const;

  TestPrefetchNetworkRequestFactory* prefetch_request_factory() {
    return &prefetch_request_factory_;
  }

  net::TestURLFetcherFactory* url_fetcher_factory() {
    return &url_fetcher_factory_;
  }

  PrefetchStore* store() { return store_test_util_.store(); }

  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }

  MockPrefetchItemGenerator* item_generator() { return &item_generator_; }

 private:
  net::TestURLFetcherFactory url_fetcher_factory_;
  TestPrefetchNetworkRequestFactory prefetch_request_factory_;
  PrefetchStoreTestUtil store_test_util_;
  MockPrefetchItemGenerator item_generator_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TASK_TEST_BASE_H_
