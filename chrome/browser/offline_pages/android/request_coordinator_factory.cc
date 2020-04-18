// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/request_coordinator_factory.h"

#include <memory>

#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_factory.h"
#include "chrome/browser/offline_pages/android/background_scheduler_bridge.h"
#include "chrome/browser/offline_pages/android/cct_request_observer.h"
#include "chrome/browser/offline_pages/android/downloads/offline_page_notification_bridge.h"
#include "chrome/browser/offline_pages/android/load_termination_listener_impl.h"
#include "chrome/browser/offline_pages/background_loader_offliner.h"
#include "chrome/browser/offline_pages/offline_page_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/offline_pages/core/background/offliner.h"
#include "components/offline_pages/core/background/offliner_policy.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/request_queue.h"
#include "components/offline_pages/core/background/request_queue_store.h"
#include "components/offline_pages/core/background/request_queue_store_sql.h"
#include "components/offline_pages/core/background/scheduler.h"
#include "components/offline_pages/core/downloads/download_notifying_observer.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/offline_pages_ukm_reporter.h"
#include "net/nqe/network_quality_estimator.h"

namespace offline_pages {

RequestCoordinatorFactory::RequestCoordinatorFactory()
    : BrowserContextKeyedServiceFactory(
          "OfflineRequestCoordinator",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(OfflinePageModelFactory::GetInstance());
}

// static
RequestCoordinatorFactory* RequestCoordinatorFactory::GetInstance() {
  return base::Singleton<RequestCoordinatorFactory>::get();
}

// static
RequestCoordinator* RequestCoordinatorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<RequestCoordinator*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* RequestCoordinatorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<OfflinerPolicy> policy(new OfflinerPolicy());
  std::unique_ptr<Offliner> offliner;
  OfflinePageModel* model =
      OfflinePageModelFactory::GetInstance()->GetForBrowserContext(context);

  std::unique_ptr<LoadTerminationListenerImpl> load_termination_listener =
      std::make_unique<LoadTerminationListenerImpl>();
  offliner.reset(new BackgroundLoaderOffliner(
      context, policy.get(), model, std::move(load_termination_listener)));

  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
  Profile* profile = Profile::FromBrowserContext(context);
  base::FilePath queue_store_path =
      profile->GetPath().Append(chrome::kOfflinePageRequestQueueDirname);

  std::unique_ptr<RequestQueueStoreSQL> queue_store(
      new RequestQueueStoreSQL(background_task_runner, queue_store_path));
  std::unique_ptr<RequestQueue> queue(new RequestQueue(std::move(queue_store)));
  std::unique_ptr<Scheduler>
      scheduler(new android::BackgroundSchedulerBridge());
  net::NetworkQualityEstimator::NetworkQualityProvider*
      network_quality_estimator =
          UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
  std::unique_ptr<OfflinePagesUkmReporter> ukm_reporter(
      new OfflinePagesUkmReporter());
  RequestCoordinator* request_coordinator = new RequestCoordinator(
      std::move(policy), std::move(offliner), std::move(queue),
      std::move(scheduler), network_quality_estimator, std::move(ukm_reporter));

  DownloadNotifyingObserver::CreateAndStartObserving(
      request_coordinator,
      std::make_unique<android::OfflinePageNotificationBridge>());

  CCTRequestObserver::AttachToRequestCoordinator(request_coordinator);

  return request_coordinator;
}

}  // namespace offline_pages
