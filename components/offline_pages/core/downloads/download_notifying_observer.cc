// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/download_notifying_observer.h"

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/downloads/download_ui_adapter.h"
#include "components/offline_pages/core/downloads/offline_item_conversions.h"
#include "components/offline_pages/core/downloads/offline_page_download_notifier.h"

namespace offline_pages {
namespace {
int kUserDataKey;  // Only address is used.
}  // namespace

DownloadNotifyingObserver::DownloadNotifyingObserver(
    std::unique_ptr<OfflinePageDownloadNotifier> notifier,
    ClientPolicyController* policy_controller)
    : notifier_(std::move(notifier)), policy_controller_(policy_controller) {}

DownloadNotifyingObserver::~DownloadNotifyingObserver() {}

// static
DownloadNotifyingObserver* DownloadNotifyingObserver::GetFromRequestCoordinator(
    RequestCoordinator* request_coordinator) {
  DCHECK(request_coordinator);
  return static_cast<DownloadNotifyingObserver*>(
      request_coordinator->GetUserData(&kUserDataKey));
}

// static
void DownloadNotifyingObserver::CreateAndStartObserving(
    RequestCoordinator* request_coordinator,
    std::unique_ptr<OfflinePageDownloadNotifier> notifier) {
  DCHECK(request_coordinator);
  DCHECK(notifier);
  std::unique_ptr<DownloadNotifyingObserver> observer =
      base::WrapUnique(new DownloadNotifyingObserver(
          std::move(notifier), request_coordinator->GetPolicyController()));
  request_coordinator->AddObserver(observer.get());
  request_coordinator->SetUserData(&kUserDataKey, std::move(observer));
}

void DownloadNotifyingObserver::OnAdded(const SavePageRequest& request) {
  DCHECK(notifier_);
  if (!IsVisibleInUI(request.client_id()))
    return;

  // Calling Progress ensures notification is created in lieu of specific
  // Add/Create call.
  notifier_->NotifyDownloadProgress(
      OfflineItemConversions::CreateOfflineItem(request));

  // Now we need to update the notification if it is not active/offlining.
  if (request.request_state() != SavePageRequest::RequestState::OFFLINING)
    NotifyRequestStateChange(request);
}

void DownloadNotifyingObserver::OnChanged(const SavePageRequest& request) {
  DCHECK(notifier_);
  if (!IsVisibleInUI(request.client_id()))
    return;
  NotifyRequestStateChange(request);
}

void DownloadNotifyingObserver::OnNetworkProgress(
    const SavePageRequest& request,
    int64_t received_bytes) {
  // TODO(dimich): Enable this back in M59. See bug 704049 for more info and
  // what was temporarily (for M58) reverted.
}

void DownloadNotifyingObserver::OnCompleted(
    const SavePageRequest& request,
    RequestCoordinator::BackgroundSavePageResult status) {
  DCHECK(notifier_);
  if (!IsVisibleInUI(request.client_id()))
    return;
  if (status == RequestCoordinator::BackgroundSavePageResult::SUCCESS) {
    // Suppress notifications for certin downloads resulting from CCT.
    OfflineItem item = OfflineItemConversions::CreateOfflineItem(request);
    if (!notifier_->MaybeSuppressNotification(request.request_origin(), item)) {
      notifier_->NotifyDownloadSuccessful(item);
    }
  } else if (status ==
                 RequestCoordinator::BackgroundSavePageResult::USER_CANCELED ||
             status == RequestCoordinator::BackgroundSavePageResult::
                           DOWNLOAD_THROTTLED) {
    notifier_->NotifyDownloadCanceled(
        OfflineItemConversions::CreateOfflineItem(request));
  } else {
    notifier_->NotifyDownloadFailed(
        OfflineItemConversions::CreateOfflineItem(request));
  }
}

bool DownloadNotifyingObserver::IsVisibleInUI(const ClientId& page) {
  return policy_controller_->IsSupportedByDownload(page.name_space) &&
         base::IsValidGUID(page.id);
}

// Calls the appropriate notifier method depending upon the state of the
// request. For example, an AVAILABLE request is not active (aka, pending)
// which the notifier understands as an Interrupted operation vs. one that
// has Progress or is Paused.
void DownloadNotifyingObserver::NotifyRequestStateChange(
    const SavePageRequest& request) {
  if (request.request_state() == SavePageRequest::RequestState::PAUSED)
    notifier_->NotifyDownloadPaused(
        OfflineItemConversions::CreateOfflineItem(request));
  else if (request.request_state() == SavePageRequest::RequestState::AVAILABLE)
    notifier_->NotifyDownloadInterrupted(
        OfflineItemConversions::CreateOfflineItem(request));
  else
    notifier_->NotifyDownloadProgress(
        OfflineItemConversions::CreateOfflineItem(request));
}

}  // namespace offline_pages
