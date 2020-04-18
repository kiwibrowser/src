// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/fresh_offline_content_observer.h"

#include "chrome/browser/offline_pages/prefetch/prefetched_pages_notifier.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

namespace {
int kFreshOfflineContentObserverUserDataKey;
}

// static
void FreshOfflineContentObserver::AttachToOfflinePageModel(
    OfflinePageModel* model) {
  if (!IsOfflinePagesPrefetchingUIEnabled())
    return;
  auto observer = std::make_unique<FreshOfflineContentObserver>();
  model->AddObserver(observer.get());
  model->SetUserData(&kFreshOfflineContentObserverUserDataKey,
                     std::move(observer));
}

FreshOfflineContentObserver::FreshOfflineContentObserver() = default;

FreshOfflineContentObserver::~FreshOfflineContentObserver() = default;

void FreshOfflineContentObserver::OfflinePageModelLoaded(
    OfflinePageModel* model) {}

void FreshOfflineContentObserver::OfflinePageAdded(
    OfflinePageModel* model,
    const OfflinePageItem& added_page) {
  if (model->GetPolicyController()->IsSupportedByDownload(
          added_page.client_id.name_space)) {
    OnFreshOfflineContentAvailableForNotification();
  }
}

void FreshOfflineContentObserver::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {}

}  // namespace offline_pages
