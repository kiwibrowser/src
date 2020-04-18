// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_DOWNLOAD_NOTIFYING_OBSERVER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_DOWNLOAD_NOTIFYING_OBSERVER_H_

#include <memory>

#include "base/guid.h"
#include "base/macros.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/client_policy_controller.h"

namespace offline_pages {

struct ClientId;
struct OfflinePageDownloadNotifier;
class ClientPolicyController;
class SavePageRequest;

// Class observing the save page requests and issuing corresponding user
// notifications as requests are added or updated.
class DownloadNotifyingObserver : public RequestCoordinator::Observer,
                                  public base::SupportsUserData::Data {
 public:
  ~DownloadNotifyingObserver() override;

  static DownloadNotifyingObserver* GetFromRequestCoordinator(
      RequestCoordinator* request_coordinator);
  static void CreateAndStartObserving(
      RequestCoordinator* request_coordinator,
      std::unique_ptr<OfflinePageDownloadNotifier> notifier);

  // RequestCoordinator::Observer implementation:
  void OnAdded(const SavePageRequest& request) override;
  void OnChanged(const SavePageRequest& request) override;
  void OnCompleted(
      const SavePageRequest& request,
      RequestCoordinator::BackgroundSavePageResult status) override;
  void OnNetworkProgress(const SavePageRequest& request,
                         int64_t received_bytes) override;

 private:
  friend class DownloadNotifyingObserverTest;

  DownloadNotifyingObserver(
      std::unique_ptr<OfflinePageDownloadNotifier> notifier,
      ClientPolicyController* policy_controller);

  bool IsVisibleInUI(const ClientId& id);

  void NotifyRequestStateChange(const SavePageRequest& request);

  // Used to issue notifications related to save page requests.
  std::unique_ptr<OfflinePageDownloadNotifier> notifier_;
  // Used to determine policy-related permissions. Not owned.
  ClientPolicyController* policy_controller_;

  DISALLOW_COPY_AND_ASSIGN(DownloadNotifyingObserver);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_DOWNLOAD_NOTIFYING_OBSERVER_H_
