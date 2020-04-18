// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/test_request_coordinator_builder.h"

#include <utility>

#include "components/offline_pages/core/background/network_quality_provider_stub.h"
#include "components/offline_pages/core/background/offliner_policy.h"
#include "components/offline_pages/core/background/offliner_stub.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/request_queue.h"
#include "components/offline_pages/core/background/request_queue_in_memory_store.h"
#include "components/offline_pages/core/background/scheduler_stub.h"
#include "components/offline_pages/core/offline_pages_ukm_reporter_stub.h"
#include "content/public/browser/browser_context.h"

namespace offline_pages {

std::unique_ptr<KeyedService> BuildTestRequestCoordinator(
    content::BrowserContext* context) {
  // Use original policy.
  std::unique_ptr<OfflinerPolicy> policy(new OfflinerPolicy());

  // Use the in-memory store.
  std::unique_ptr<RequestQueueInMemoryStore> store(
      new RequestQueueInMemoryStore());
  // Use the regular test queue (should work).
  std::unique_ptr<RequestQueue> queue(new RequestQueue(std::move(store)));

  // Initialize the rest with stubs.
  std::unique_ptr<Offliner> offliner(new OfflinerStub());
  std::unique_ptr<Scheduler> scheduler_stub(new SchedulerStub());

  // NetworkQualityProviderStub should be set by the test on the context first.
  NetworkQualityProviderStub* network_quality_provider =
      NetworkQualityProviderStub::GetUserData(context);

  std::unique_ptr<OfflinePagesUkmReporter> ukm_reporter_stub(
      new OfflinePagesUkmReporterStub());

  return std::unique_ptr<RequestCoordinator>(new RequestCoordinator(
      std::move(policy), std::move(offliner), std::move(queue),
      std::move(scheduler_stub), network_quality_provider,
      std::move(ukm_reporter_stub)));
}

}  // namespace offline_pages
