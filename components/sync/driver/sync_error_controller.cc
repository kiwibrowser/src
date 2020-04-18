// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_error_controller.h"

#include "components/sync/driver/sync_service.h"

namespace syncer {

SyncErrorController::SyncErrorController(SyncService* service)
    : service_(service) {
  DCHECK(service_);
}

SyncErrorController::~SyncErrorController() {}

bool SyncErrorController::HasError() {
  return service_->IsFirstSetupComplete() && service_->IsPassphraseRequired() &&
         service_->IsPassphraseRequiredForDecryption();
}

void SyncErrorController::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void SyncErrorController::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void SyncErrorController::OnStateChanged(syncer::SyncService* sync) {
  for (auto& observer : observer_list_)
    observer.OnErrorChanged();
}

}  // namespace syncer
