// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TOOLS_NULL_INVALIDATION_STATE_TRACKER_H_
#define COMPONENTS_SYNC_TOOLS_NULL_INVALIDATION_STATE_TRACKER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/invalidation/impl/invalidation_state_tracker.h"

namespace syncer {

class NullInvalidationStateTracker
    : public base::SupportsWeakPtr<NullInvalidationStateTracker>,
      public InvalidationStateTracker {
 public:
  NullInvalidationStateTracker();
  ~NullInvalidationStateTracker() override;

  void ClearAndSetNewClientId(const std::string& data) override;
  std::string GetInvalidatorClientId() const override;

  std::string GetBootstrapData() const override;
  void SetBootstrapData(const std::string& data) override;

  void SetSavedInvalidations(const UnackedInvalidationsMap& states) override;
  UnackedInvalidationsMap GetSavedInvalidations() const override;

  void Clear() override;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TOOLS_NULL_INVALIDATION_STATE_TRACKER_H_
