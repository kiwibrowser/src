// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/fake_invalidation_state_tracker.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

const int64_t FakeInvalidationStateTracker::kMinVersion = INT64_MIN;

FakeInvalidationStateTracker::FakeInvalidationStateTracker() {}

FakeInvalidationStateTracker::~FakeInvalidationStateTracker() {}

void FakeInvalidationStateTracker::ClearAndSetNewClientId(
    const std::string& client_id) {
  Clear();
  invalidator_client_id_ = client_id;
}

std::string FakeInvalidationStateTracker::GetInvalidatorClientId() const {
  return invalidator_client_id_;
}

void FakeInvalidationStateTracker::SetBootstrapData(
    const std::string& data) {
  bootstrap_data_ = data;
}

std::string FakeInvalidationStateTracker::GetBootstrapData() const {
  return bootstrap_data_;
}

void FakeInvalidationStateTracker::SetSavedInvalidations(
    const UnackedInvalidationsMap& states) {
  unacked_invalidations_map_ = states;
}

UnackedInvalidationsMap
FakeInvalidationStateTracker::GetSavedInvalidations() const {
  return unacked_invalidations_map_;
}

void FakeInvalidationStateTracker::Clear() {
  invalidator_client_id_.clear();
  bootstrap_data_.clear();
}

}  // namespace syncer
