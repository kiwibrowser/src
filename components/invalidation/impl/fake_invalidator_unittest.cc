// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/fake_invalidator.h"

#include <memory>

#include "base/compiler_specific.h"
#include "components/invalidation/impl/invalidator_test_template.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

class FakeInvalidatorTestDelegate {
 public:
  FakeInvalidatorTestDelegate() {}

  ~FakeInvalidatorTestDelegate() {
    DestroyInvalidator();
  }

  void CreateInvalidator(
      const std::string& invalidator_client_id,
      const std::string& initial_state,
      const base::WeakPtr<InvalidationStateTracker>&
          invalidation_state_tracker) {
    DCHECK(!invalidator_);
    invalidator_.reset(new FakeInvalidator());
  }

  FakeInvalidator* GetInvalidator() {
    return invalidator_.get();
  }

  void DestroyInvalidator() {
    invalidator_.reset();
  }

  void WaitForInvalidator() {
    // Do Nothing.
  }

  void TriggerOnInvalidatorStateChange(InvalidatorState state) {
    invalidator_->EmitOnInvalidatorStateChange(state);
  }

  void TriggerOnIncomingInvalidation(
      const ObjectIdInvalidationMap& invalidation_map) {
    invalidator_->EmitOnIncomingInvalidation(invalidation_map);
  }

 private:
  std::unique_ptr<FakeInvalidator> invalidator_;
};

INSTANTIATE_TYPED_TEST_CASE_P(
    FakeInvalidatorTest, InvalidatorTest,
    FakeInvalidatorTestDelegate);

}  // namespace

}  // namespace syncer
