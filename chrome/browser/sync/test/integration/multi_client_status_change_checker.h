// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_MULTI_CLIENT_STATUS_CHANGE_CHECKER_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_MULTI_CLIENT_STATUS_CHANGE_CHECKER_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "chrome/browser/sync/test/integration/status_change_checker.h"
#include "components/sync/driver/sync_service_observer.h"

namespace browser_sync {
class ProfileSyncService;
}  // namespace browser_sync

// This class provides some common functionality for StatusChangeCheckers that
// observe many ProfileSyncServices.  This class is abstract.  Its descendants
// are expected to provide additional functionality.
class MultiClientStatusChangeChecker : public StatusChangeChecker,
                                       public syncer::SyncServiceObserver {
 public:
  explicit MultiClientStatusChangeChecker(
      std::vector<browser_sync::ProfileSyncService*> services);
  ~MultiClientStatusChangeChecker() override;

  // Called when waiting times out.
  void OnTimeout();

  // syncer::SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* sync) override;

  // StatusChangeChecker implementations and stubs.
  bool IsExitConditionSatisfied() override = 0;
  std::string GetDebugMessage() const override = 0;

 protected:
  base::TimeDelta GetTimeoutDuration() override;

  const std::vector<browser_sync::ProfileSyncService*>& services() {
    return services_;
  }

 private:
  std::vector<browser_sync::ProfileSyncService*> services_;
  ScopedObserver<browser_sync::ProfileSyncService,
                 MultiClientStatusChangeChecker>
      scoped_observer_;
};

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_MULTI_CLIENT_STATUS_CHANGE_CHECKER_H_
