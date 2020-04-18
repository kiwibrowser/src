// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_FAKE_INVALIDATION_STATE_TRACKER_H_
#define COMPONENTS_INVALIDATION_IMPL_FAKE_INVALIDATION_STATE_TRACKER_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "components/invalidation/impl/invalidation_state_tracker.h"

namespace syncer {

// InvalidationStateTracker implementation that simply keeps track of
// the max versions and invalidation state in memory.
class FakeInvalidationStateTracker
    : public InvalidationStateTracker,
      public base::SupportsWeakPtr<FakeInvalidationStateTracker> {
 public:
  FakeInvalidationStateTracker();
  ~FakeInvalidationStateTracker() override;

  // InvalidationStateTracker implementation.
  void ClearAndSetNewClientId(const std::string& client_id) override;
  std::string GetInvalidatorClientId() const override;
  void SetBootstrapData(const std::string& data) override;
  std::string GetBootstrapData() const override;
  void SetSavedInvalidations(const UnackedInvalidationsMap& states) override;
  UnackedInvalidationsMap GetSavedInvalidations() const override;
  void Clear() override;

  static const int64_t kMinVersion;

 private:
  std::string invalidator_client_id_;
  std::string bootstrap_data_;
  UnackedInvalidationsMap unacked_invalidations_map_;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_FAKE_INVALIDATION_STATE_TRACKER_H_
