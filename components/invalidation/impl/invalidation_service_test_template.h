// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class defines tests that implementations of InvalidationService should
// pass in order to be conformant.  Here's how you use it to test your
// implementation.
//
// Say your class is called MyInvalidationService.  Then you need to define a
// class called MyInvalidationServiceTestDelegate in
// my_invalidation_frontend_unittest.cc like this:
//
//   class MyInvalidationServiceTestDelegate {
//    public:
//     MyInvalidationServiceTestDelegate() ...
//
//     ~MyInvalidationServiceTestDelegate() {
//       // DestroyInvalidator() may not be explicitly called by tests.
//       DestroyInvalidator();
//     }
//
//     // Create the InvalidationService implementation with the given params.
//     void CreateInvalidationService() {
//       ...
//     }
//
//     // Should return the InvalidationService implementation.  Only called
//     // after CreateInvalidator and before DestroyInvalidator.
//     MyInvalidationService* GetInvalidationService() {
//       ...
//     }
//
//     // Destroy the InvalidationService implementation.
//     void DestroyInvalidationService() {
//       ...
//     }
//
//     // The Trigger* functions below should block until the effects of
//     // the call are visible on the current thread.
//
//     // Should cause OnInvalidatorStateChange() to be called on all
//     // observers of the InvalidationService implementation with the given
//     // parameters.
//     void TriggerOnInvalidatorStateChange(InvalidatorState state) {
//       ...
//     }
//
//     // Should cause OnIncomingInvalidation() to be called on all
//     // observers of the InvalidationService implementation with the given
//     // parameters.
//     void TriggerOnIncomingInvalidation(
//         const ObjectIdInvalidationMap& invalidation_map) {
//       ...
//     }
//   };
//
// The InvalidationServiceTest test harness will have a member variable of
// this delegate type and will call its functions in the various
// tests.
//
// Then you simply #include this file as well as gtest.h and add the
// following statement to my_sync_notifier_unittest.cc:
//
//   INSTANTIATE_TYPED_TEST_CASE_P(
//       MyInvalidationService,
//       InvalidationServiceTest,
//       MyInvalidatorTestDelegate);
//
// Easy!

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_TEST_TEMPLATE_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_TEST_TEMPLATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/invalidation/impl/fake_invalidation_handler.h"
#include "components/invalidation/impl/object_id_invalidation_map_test_util.h"
#include "components/invalidation/public/ack_handle.h"
#include "components/invalidation/public/invalidation.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/types.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

template <typename InvalidatorTestDelegate>
class InvalidationServiceTest : public testing::Test {
 protected:
  InvalidationServiceTest()
      : id1(ipc::invalidation::ObjectSource::CHROME_SYNC, "BOOKMARK"),
        id2(ipc::invalidation::ObjectSource::CHROME_SYNC, "PREFERENCE"),
        id3(ipc::invalidation::ObjectSource::CHROME_SYNC, "AUTOFILL"),
        id4(ipc::invalidation::ObjectSource::CHROME_PUSH_MESSAGING,
            "PUSH_MESSAGE") {
  }

  invalidation::InvalidationService*
  CreateAndInitializeInvalidationService() {
    this->delegate_.CreateInvalidationService();
    return this->delegate_.GetInvalidationService();
  }

  InvalidatorTestDelegate delegate_;

  const invalidation::ObjectId id1;
  const invalidation::ObjectId id2;
  const invalidation::ObjectId id3;
  const invalidation::ObjectId id4;
};

TYPED_TEST_CASE_P(InvalidationServiceTest);

// Initialize the invalidator, register a handler, register some IDs for that
// handler, and then unregister the handler, dispatching invalidations in
// between.  The handler should only see invalidations when its registered and
// its IDs are registered.
TYPED_TEST_P(InvalidationServiceTest, Basic) {
  invalidation::InvalidationService* const invalidator =
      this->CreateAndInitializeInvalidationService();

  syncer::FakeInvalidationHandler handler;

  invalidator->RegisterInvalidationHandler(&handler);

  syncer::ObjectIdInvalidationMap invalidation_map;
  invalidation_map.Insert(syncer::Invalidation::Init(this->id1, 1, "1"));
  invalidation_map.Insert(syncer::Invalidation::Init(this->id2, 2, "2"));
  invalidation_map.Insert(syncer::Invalidation::Init(this->id3, 3, "3"));

  // Should be ignored since no IDs are registered to |handler|.
  this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);
  EXPECT_EQ(0, handler.GetInvalidationCount());

  syncer::ObjectIdSet ids;
  ids.insert(this->id1);
  ids.insert(this->id2);
  EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler, ids));

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::INVALIDATIONS_ENABLED);
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler.GetInvalidatorState());

  syncer::ObjectIdInvalidationMap expected_invalidations;
  expected_invalidations.Insert(syncer::Invalidation::Init(this->id1, 1, "1"));
  expected_invalidations.Insert(syncer::Invalidation::Init(this->id2, 2, "2"));

  this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);
  EXPECT_EQ(1, handler.GetInvalidationCount());
  EXPECT_THAT(expected_invalidations, Eq(handler.GetLastInvalidationMap()));

  ids.erase(this->id1);
  ids.insert(this->id3);
  EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler, ids));

  expected_invalidations = syncer::ObjectIdInvalidationMap();
  expected_invalidations.Insert(syncer::Invalidation::Init(this->id2, 2, "2"));
  expected_invalidations.Insert(syncer::Invalidation::Init(this->id3, 3, "3"));

  // Removed object IDs should not be notified, newly-added ones should.
  this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);
  EXPECT_EQ(2, handler.GetInvalidationCount());
  EXPECT_THAT(expected_invalidations, Eq(handler.GetLastInvalidationMap()));

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::TRANSIENT_INVALIDATION_ERROR);
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler.GetInvalidatorState());

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::INVALIDATIONS_ENABLED);
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED,
            handler.GetInvalidatorState());

  invalidator->UnregisterInvalidationHandler(&handler);

  // Should be ignored since |handler| isn't registered anymore.
  this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);
  EXPECT_EQ(2, handler.GetInvalidationCount());
}

// Register handlers and some IDs for those handlers, register a handler with
// no IDs, and register a handler with some IDs but unregister it.  Then,
// dispatch some invalidations and invalidations.  Handlers that are registered
// should get invalidations, and the ones that have registered IDs should
// receive invalidations for those IDs.
TYPED_TEST_P(InvalidationServiceTest, MultipleHandlers) {
  invalidation::InvalidationService* const invalidator =
      this->CreateAndInitializeInvalidationService();

  syncer::FakeInvalidationHandler handler1;
  syncer::FakeInvalidationHandler handler2;
  syncer::FakeInvalidationHandler handler3;
  syncer::FakeInvalidationHandler handler4;

  invalidator->RegisterInvalidationHandler(&handler1);
  invalidator->RegisterInvalidationHandler(&handler2);
  invalidator->RegisterInvalidationHandler(&handler3);
  invalidator->RegisterInvalidationHandler(&handler4);

  {
    syncer::ObjectIdSet ids;
    ids.insert(this->id1);
    ids.insert(this->id2);
    EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler1, ids));
  }

  {
    syncer::ObjectIdSet ids;
    ids.insert(this->id3);
    EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler2, ids));
  }

  // Don't register any IDs for handler3.

  {
    syncer::ObjectIdSet ids;
    ids.insert(this->id4);
    EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler4, ids));
  }

  invalidator->UnregisterInvalidationHandler(&handler4);

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::INVALIDATIONS_ENABLED);
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler1.GetInvalidatorState());
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler2.GetInvalidatorState());
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler3.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler4.GetInvalidatorState());

  {
    syncer::ObjectIdInvalidationMap invalidation_map;
    invalidation_map.Insert(syncer::Invalidation::Init(this->id1, 1, "1"));
    invalidation_map.Insert(syncer::Invalidation::Init(this->id2, 2, "2"));
    invalidation_map.Insert(syncer::Invalidation::Init(this->id3, 3, "3"));
    invalidation_map.Insert(syncer::Invalidation::Init(this->id4, 4, "4"));
    this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);

    syncer::ObjectIdInvalidationMap expected_invalidations;
    expected_invalidations.Insert(
        syncer::Invalidation::Init(this->id1, 1, "1"));
    expected_invalidations.Insert(
        syncer::Invalidation::Init(this->id2, 2, "2"));

    EXPECT_EQ(1, handler1.GetInvalidationCount());
    EXPECT_THAT(expected_invalidations, Eq(handler1.GetLastInvalidationMap()));

    expected_invalidations = syncer::ObjectIdInvalidationMap();
    expected_invalidations.Insert(
        syncer::Invalidation::Init(this->id3, 3, "3"));

    EXPECT_EQ(1, handler2.GetInvalidationCount());
    EXPECT_THAT(expected_invalidations, Eq(handler2.GetLastInvalidationMap()));

    EXPECT_EQ(0, handler3.GetInvalidationCount());
    EXPECT_EQ(0, handler4.GetInvalidationCount());
  }

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::TRANSIENT_INVALIDATION_ERROR);
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler1.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler2.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler3.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler4.GetInvalidatorState());

  invalidator->UnregisterInvalidationHandler(&handler3);
  invalidator->UnregisterInvalidationHandler(&handler2);
  invalidator->UnregisterInvalidationHandler(&handler1);
}

// Multiple registrations by different handlers on the same object ID should
// return false.
TYPED_TEST_P(InvalidationServiceTest, MultipleRegistrations) {
  invalidation::InvalidationService* const invalidator =
      this->CreateAndInitializeInvalidationService();

  syncer::FakeInvalidationHandler handler1;
  syncer::FakeInvalidationHandler handler2;

  invalidator->RegisterInvalidationHandler(&handler1);
  invalidator->RegisterInvalidationHandler(&handler2);

  // Registering both handlers for the same ObjectId. First call should succeed,
  // second should fail.
  syncer::ObjectIdSet ids;
  ids.insert(this->id1);
  EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler1, ids));
  EXPECT_FALSE(invalidator->UpdateRegisteredInvalidationIds(&handler2, ids));

  invalidator->UnregisterInvalidationHandler(&handler2);
  invalidator->UnregisterInvalidationHandler(&handler1);
}

// Make sure that passing an empty set to UpdateRegisteredInvalidationIds clears
// the corresponding entries for the handler.
TYPED_TEST_P(InvalidationServiceTest, EmptySetUnregisters) {
  invalidation::InvalidationService* const invalidator =
      this->CreateAndInitializeInvalidationService();

  syncer::FakeInvalidationHandler handler1;

  // Control observer.
  syncer::FakeInvalidationHandler handler2;

  invalidator->RegisterInvalidationHandler(&handler1);
  invalidator->RegisterInvalidationHandler(&handler2);

  {
    syncer::ObjectIdSet ids;
    ids.insert(this->id1);
    ids.insert(this->id2);
    EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler1, ids));
  }

  {
    syncer::ObjectIdSet ids;
    ids.insert(this->id3);
    EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(&handler2, ids));
  }

  // Unregister the IDs for the first observer. It should not receive any
  // further invalidations.
  EXPECT_TRUE(invalidator->UpdateRegisteredInvalidationIds(
      &handler1, syncer::ObjectIdSet()));

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::INVALIDATIONS_ENABLED);
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler1.GetInvalidatorState());
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler2.GetInvalidatorState());

  {
    syncer::ObjectIdInvalidationMap invalidation_map;
    invalidation_map.Insert(syncer::Invalidation::Init(this->id1, 1, "1"));
    invalidation_map.Insert(syncer::Invalidation::Init(this->id2, 2, "2"));
    invalidation_map.Insert(syncer::Invalidation::Init(this->id3, 3, "3"));
    this->delegate_.TriggerOnIncomingInvalidation(invalidation_map);
    EXPECT_EQ(0, handler1.GetInvalidationCount());
    EXPECT_EQ(1, handler2.GetInvalidationCount());
  }

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::TRANSIENT_INVALIDATION_ERROR);
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler1.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler2.GetInvalidatorState());

  invalidator->UnregisterInvalidationHandler(&handler2);
  invalidator->UnregisterInvalidationHandler(&handler1);
}

namespace internal {

// A FakeInvalidationHandler that is "bound" to a specific
// InvalidationService.  This is for cross-referencing state information with
// the bound InvalidationService.
class BoundFakeInvalidationHandler : public syncer::FakeInvalidationHandler {
 public:
  explicit BoundFakeInvalidationHandler(
      const invalidation::InvalidationService& invalidator);
  ~BoundFakeInvalidationHandler() override;

  // Returns the last return value of GetInvalidatorState() on the
  // bound invalidator from the last time the invalidator state
  // changed.
  syncer::InvalidatorState GetLastRetrievedState() const;

  // InvalidationHandler implementation.
  void OnInvalidatorStateChange(syncer::InvalidatorState state) override;

 private:
  const invalidation::InvalidationService& invalidator_;
  syncer::InvalidatorState last_retrieved_state_;

  DISALLOW_COPY_AND_ASSIGN(BoundFakeInvalidationHandler);
};

}  // namespace internal

TYPED_TEST_P(InvalidationServiceTest, GetInvalidatorStateAlwaysCurrent) {
  invalidation::InvalidationService* const invalidator =
      this->CreateAndInitializeInvalidationService();

  internal::BoundFakeInvalidationHandler handler(*invalidator);
  invalidator->RegisterInvalidationHandler(&handler);

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::INVALIDATIONS_ENABLED);
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler.GetInvalidatorState());
  EXPECT_EQ(syncer::INVALIDATIONS_ENABLED, handler.GetLastRetrievedState());

  this->delegate_.TriggerOnInvalidatorStateChange(
      syncer::TRANSIENT_INVALIDATION_ERROR);
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler.GetInvalidatorState());
  EXPECT_EQ(syncer::TRANSIENT_INVALIDATION_ERROR,
            handler.GetLastRetrievedState());

  invalidator->UnregisterInvalidationHandler(&handler);
}

REGISTER_TYPED_TEST_CASE_P(InvalidationServiceTest,
                           Basic,
                           MultipleHandlers,
                           MultipleRegistrations,
                           EmptySetUnregisters,
                           GetInvalidatorStateAlwaysCurrent);

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_TEST_TEMPLATE_H_
