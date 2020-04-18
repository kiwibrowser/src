// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidator_test_template.h"

namespace syncer {
namespace internal {

BoundFakeInvalidationHandler::BoundFakeInvalidationHandler(
    const Invalidator& invalidator)
    : invalidator_(invalidator),
      last_retrieved_state_(DEFAULT_INVALIDATION_ERROR) {}

BoundFakeInvalidationHandler::~BoundFakeInvalidationHandler() {}

InvalidatorState BoundFakeInvalidationHandler::GetLastRetrievedState() const {
  return last_retrieved_state_;
}

void BoundFakeInvalidationHandler::OnInvalidatorStateChange(
    InvalidatorState state) {
  FakeInvalidationHandler::OnInvalidatorStateChange(state);
  last_retrieved_state_ = invalidator_.GetInvalidatorState();
}

}  // namespace internal
}  // namespace syncer
