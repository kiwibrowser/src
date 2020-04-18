// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidator_registrar.h"

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/invalidation/impl/fake_invalidation_handler.h"
#include "components/invalidation/impl/invalidator_test_template.h"
#include "google/cacheinvalidation/types.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

// We test InvalidatorRegistrar by wrapping it in an Invalidator and
// running the usual Invalidator tests.

// Thin Invalidator wrapper around InvalidatorRegistrar.
class RegistrarInvalidator : public Invalidator {
 public:
  RegistrarInvalidator() {}
  ~RegistrarInvalidator() override {}

  InvalidatorRegistrar* GetRegistrar() {
    return &registrar_;
  }

  // Invalidator implementation.
  void RegisterHandler(InvalidationHandler* handler) override {
    registrar_.RegisterHandler(handler);
  }

  bool UpdateRegisteredIds(InvalidationHandler* handler,
                           const ObjectIdSet& ids) override {
    return registrar_.UpdateRegisteredIds(handler, ids);
  }

  void UnregisterHandler(InvalidationHandler* handler) override {
    registrar_.UnregisterHandler(handler);
  }

  InvalidatorState GetInvalidatorState() const override {
    return registrar_.GetInvalidatorState();
  }

  void UpdateCredentials(const std::string& email,
                         const std::string& token) override {
    // Do nothing.
  }

  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> call) const override {
    // Do nothing.
  }

 private:
  InvalidatorRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(RegistrarInvalidator);
};

class RegistrarInvalidatorTestDelegate {
 public:
  RegistrarInvalidatorTestDelegate() {}

  ~RegistrarInvalidatorTestDelegate() {
    DestroyInvalidator();
  }

  void CreateInvalidator(
      const std::string& invalidator_client_id,
      const std::string& initial_state,
      const base::WeakPtr<InvalidationStateTracker>&
          invalidation_state_tracker) {
    DCHECK(!invalidator_);
    invalidator_.reset(new RegistrarInvalidator());
  }

  RegistrarInvalidator* GetInvalidator() {
    return invalidator_.get();
  }

  void DestroyInvalidator() {
    invalidator_.reset();
  }

  void WaitForInvalidator() {
    // Do nothing.
  }

  void TriggerOnInvalidatorStateChange(InvalidatorState state) {
    invalidator_->GetRegistrar()->UpdateInvalidatorState(state);
  }

  void TriggerOnIncomingInvalidation(
      const ObjectIdInvalidationMap& invalidation_map) {
    invalidator_->GetRegistrar()->DispatchInvalidationsToHandlers(
        invalidation_map);
  }

 private:
  std::unique_ptr<RegistrarInvalidator> invalidator_;
};

INSTANTIATE_TYPED_TEST_CASE_P(
    RegistrarInvalidatorTest, InvalidatorTest,
    RegistrarInvalidatorTestDelegate);

}  // namespace

}  // namespace syncer
