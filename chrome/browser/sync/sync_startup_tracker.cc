// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_startup_tracker.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"

SyncStartupTracker::SyncStartupTracker(Profile* profile, Observer* observer)
    : profile_(profile),
      observer_(observer) {
  browser_sync::ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (service)
    service->AddObserver(this);

  CheckServiceState();
}

SyncStartupTracker::~SyncStartupTracker() {
  browser_sync::ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (service)
    service->RemoveObserver(this);
}

void SyncStartupTracker::OnStateChanged(syncer::SyncService* sync) {
  CheckServiceState();
}

void SyncStartupTracker::CheckServiceState() {
  // Note: the observer may free this object so it is not allowed to access
  // this object after invoking the observer callback below.
  switch (GetSyncServiceState(profile_)) {
    case SYNC_STARTUP_ERROR:
      observer_->SyncStartupFailed();
      break;
    case SYNC_STARTUP_COMPLETE:
      observer_->SyncStartupCompleted();
      break;
    case SYNC_STARTUP_PENDING:
      // Do nothing - still waiting for sync to finish starting up.
      break;
  }
}

// static
SyncStartupTracker::SyncServiceState SyncStartupTracker::GetSyncServiceState(
    Profile* profile) {
  // If sync is not allowed, treat this as a startup error.
  if (!profile->IsSyncAllowed())
    return SYNC_STARTUP_ERROR;

  browser_sync::ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile);

  // If no service exists or it can't be started, treat as a startup error.
  if (!service || !service->CanSyncStart()) {
    return SYNC_STARTUP_ERROR;
  }

  // If the sync engine has started up, notify the callback.
  if (service->IsEngineInitialized())
    return SYNC_STARTUP_COMPLETE;

  // If the sync service has some kind of error, report to the user.
  if (service->HasUnrecoverableError())
    return SYNC_STARTUP_ERROR;

  // If we have an auth error and sync is not still waiting for new auth tokens
  // to be fetched, exit.
  if (!service->waiting_for_auth() &&
      service->GetAuthError().state() != GoogleServiceAuthError::NONE) {
    return SYNC_STARTUP_ERROR;
  }

  // No error detected yet, but the sync engine hasn't started up yet, so
  // we're in the pending state.
  return SYNC_STARTUP_PENDING;
}
