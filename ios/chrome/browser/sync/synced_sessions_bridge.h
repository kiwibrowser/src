// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SYNC_SYNCED_SESSIONS_BRIDGE_H_
#define IOS_CHROME_BROWSER_SYNC_SYNCED_SESSIONS_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "components/signin/core/browser/signin_manager_base.h"
#include "components/sync/driver/sync_service_observer.h"
#import "ios/chrome/browser/sync/sync_observer_bridge.h"

namespace ios {
class ChromeBrowserState;
}
class SigninManager;

@protocol SyncedSessionsObserver<SyncObserverModelBridge>
// Reloads the session data.
- (void)reloadSessions;
@end

namespace synced_sessions {

// Bridge class that will notify the panel when the remote sessions content
// change.
class SyncedSessionsObserverBridge : public SyncObserverBridge,
                                     public SigninManagerBase::Observer {
 public:
  SyncedSessionsObserverBridge(id<SyncedSessionsObserver> owner,
                               ios::ChromeBrowserState* browserState);
  ~SyncedSessionsObserverBridge() override;
  // SyncObserverBridge implementation.
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncCycleCompleted(syncer::SyncService* sync) override;
  void OnSyncConfigurationCompleted(syncer::SyncService* sync) override;
  void OnForeignSessionUpdated(syncer::SyncService* sync) override;
  // SigninManagerBase::Observer implementation.
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;
  // Returns true if the first sync cycle that contains session information is
  // completed. Returns false otherwise.
  bool IsFirstSyncCycleCompleted();
  // Returns true if user is signed in.
  bool IsSignedIn();
  // Returns true if it is undergoing the first sync cycle.
  bool IsSyncing();
  // Check if the first sync cycle is completed.  This keeps
  // IsFirstSyncCycleCompleted() and first_sync_cycle_is_completed_ updated.
  void CheckIfFirstSyncIsCompleted();

 private:
  __weak id<SyncedSessionsObserver> owner_ = nil;
  SigninManager* signin_manager_;
  syncer::SyncService* sync_service_;
  ios::ChromeBrowserState* browser_state_;
  ScopedObserver<SigninManagerBase, SigninManagerBase::Observer>
      signin_manager_observer_;
  // Stores whether the first sync cycle that contains session information is
  // completed.
  bool first_sync_cycle_is_completed_;
};

}  // namespace synced_sessions

#endif  // IOS_CHROME_BROWSER_SYNC_SYNCED_SESSIONS_BRIDGE_H_
