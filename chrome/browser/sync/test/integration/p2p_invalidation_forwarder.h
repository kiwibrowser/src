// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_P2P_INVALIDATION_FORWARDER_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_P2P_INVALIDATION_FORWARDER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sync/driver/sync_service_observer.h"

namespace browser_sync {
class ProfileSyncService;
}  // namespace browser_sync

namespace invalidation {
class P2PInvalidationService;
}  // namespace invalidation

// This class links the ProfileSyncService to a P2PInvalidationService.
//
// It will observe ProfileSyncService events and emit invalidation events for
// any committed changes in observes.
//
// It register and unregisters in its constructor and destructor.  This is
// intended to make it easy to manage with a scoped_ptr.
class P2PInvalidationForwarder : public syncer::SyncServiceObserver {
 public:
  P2PInvalidationForwarder(
      browser_sync::ProfileSyncService* sync_service,
      invalidation::P2PInvalidationService* invalidation_service);
  ~P2PInvalidationForwarder() override;

  // Implementation of syncer::SyncServiceObserver
  void OnSyncCycleCompleted(syncer::SyncService* sync) override;

 private:
  browser_sync::ProfileSyncService* sync_service_;
  invalidation::P2PInvalidationService* invalidation_service_;

  DISALLOW_COPY_AND_ASSIGN(P2PInvalidationForwarder);
};

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_P2P_INVALIDATION_FORWARDER_H_
