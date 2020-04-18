// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_COORDINATOR_STUB_TACO_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_COORDINATOR_STUB_TACO_H_

#include <memory>

#include "components/offline_pages/core/background/offliner.h"
#include "components/offline_pages/core/background/offliner_policy.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/request_queue.h"
#include "components/offline_pages/core/background/request_queue_store.h"
#include "components/offline_pages/core/background/scheduler.h"
#include "net/nqe/network_quality_estimator.h"

namespace offline_pages {

// The taco class acts as a wrapper around the request coordinator making
// it easy to create for tests, using stub versions of the dependencies.
// This class is like a taco shell, and the filling is the request coordinator.
// The default dependencies may be replaced by the test author to provide
// custom versions that have test-specific hooks.
class RequestCoordinatorStubTaco {
 public:
  RequestCoordinatorStubTaco();
  ~RequestCoordinatorStubTaco();

  // These methods must be called before CreateRequestCoordinator() is invoked.
  // If called after they will CHECK().
  void SetOfflinerPolicy(std::unique_ptr<OfflinerPolicy> policy);
  // Note: it makes sense to only override a default store or the queue, but not
  // the both since setting the store will auto-create a default queue with it.
  // Conflicting usage will CHECK().
  void SetRequestQueueStore(std::unique_ptr<RequestQueueStore> store);
  void SetRequestQueue(std::unique_ptr<RequestQueue> queue);
  void SetOffliner(std::unique_ptr<Offliner> offliner);
  void SetScheduler(std::unique_ptr<Scheduler> scheduler);
  void SetNetworkQualityProvider(
      std::unique_ptr<net::NetworkQualityEstimator::NetworkQualityProvider>
      network_quality_provider);
  void SetOfflinePagesUkmReporter(
      std::unique_ptr<OfflinePagesUkmReporter> ukm_reporter);

  // Creates and caches an instance of RequestCoordinator, using default or
  // overridden stub dependencies.
  void CreateRequestCoordinator();

  // Once CreateRequestCoordinator() is called, this accessor method start
  // returning the RequestCoordinator.
  // If called before CreateRequestCoordinator(), it will CHECK().

  RequestCoordinator* request_coordinator();

 private:
  bool store_overridden_ = false;
  bool queue_overridden_ = false;

  std::unique_ptr<OfflinerPolicy> policy_;
  std::unique_ptr<RequestQueue> queue_;
  std::unique_ptr<Offliner> offliner_;
  std::unique_ptr<Scheduler> scheduler_;
  std::unique_ptr<net::NetworkQualityEstimator::NetworkQualityProvider>
      network_quality_provider_;
  std::unique_ptr<OfflinePagesUkmReporter> ukm_reporter_;

  std::unique_ptr<RequestCoordinator> request_coordinator_;
};
}  // namespace offline_pages
#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_COORDINATOR_STUB_TACO_H_
