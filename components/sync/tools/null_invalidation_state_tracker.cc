// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/tools/null_invalidation_state_tracker.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task_runner.h"
#include "components/invalidation/public/invalidation_util.h"

namespace syncer {

NullInvalidationStateTracker::NullInvalidationStateTracker() {}
NullInvalidationStateTracker::~NullInvalidationStateTracker() {}

void NullInvalidationStateTracker::ClearAndSetNewClientId(
    const std::string& data) {
  LOG(INFO) << "Setting invalidator client ID to: " << data;
}

std::string NullInvalidationStateTracker::GetInvalidatorClientId() const {
  // The caller of this function is probably looking for an ID it can use to
  // identify this client as the originator of some notifiable change.  It does
  // this so the invalidation server can prevent it from being notified of its
  // own changes.  This invalidation state tracker doesn't remember its ID, so
  // it can't support this feature.
  NOTREACHED() << "This state tracker does not support reflection-blocking";
  return std::string();
}

std::string NullInvalidationStateTracker::GetBootstrapData() const {
  return std::string();
}

void NullInvalidationStateTracker::SetBootstrapData(const std::string& data) {
  std::string base64_data;
  base::Base64Encode(data, &base64_data);
  LOG(INFO) << "Setting bootstrap data to: " << base64_data;
}

void NullInvalidationStateTracker::Clear() {
  // We have no members to clear.
}

void NullInvalidationStateTracker::SetSavedInvalidations(
    const UnackedInvalidationsMap& states) {
  // Do nothing.
}

UnackedInvalidationsMap NullInvalidationStateTracker::GetSavedInvalidations()
    const {
  return UnackedInvalidationsMap();
}

}  // namespace syncer
