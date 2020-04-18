// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidation_logger.h"
#include "components/invalidation/impl/invalidation_logger_observer.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace invalidation {

class InvalidationLoggerObserverTest : public InvalidationLoggerObserver {
 public:
  InvalidationLoggerObserverTest() { ResetStates(); }

  void ResetStates() {
    registration_change_received = false;
    state_received = false;
    update_id_received = false;
    debug_message_received = false;
    invalidation_received = false;
    detailed_status_received = false;
    update_id_replicated = std::map<std::string, syncer::ObjectIdCountMap>();
    registered_handlers = std::multiset<std::string>();
  }

  void OnRegistrationChange(
      const std::multiset<std::string>& handlers) override {
    registered_handlers = handlers;
    registration_change_received = true;
  }

  void OnStateChange(const syncer::InvalidatorState& new_state,
                     const base::Time& last_change_timestamp) override {
    state_received = true;
  }

  void OnUpdateIds(const std::string& handler,
                   const syncer::ObjectIdCountMap& details) override {
    update_id_received = true;
    update_id_replicated[handler] = details;
  }

  void OnDebugMessage(const base::DictionaryValue& details) override {
    debug_message_received = true;
  }

  void OnInvalidation(
      const syncer::ObjectIdInvalidationMap& new_invalidations) override {
    invalidation_received = true;
  }

  void OnDetailedStatus(const base::DictionaryValue& details) override {
    detailed_status_received = true;
  }

  bool registration_change_received;
  bool state_received;
  bool update_id_received;
  bool debug_message_received;
  bool invalidation_received;
  bool detailed_status_received;
  std::map<std::string, syncer::ObjectIdCountMap> update_id_replicated;
  std::multiset<std::string> registered_handlers;
};

// Test that the callbacks are actually being called when observers are
// registered and don't produce any other callback in the meantime.
TEST(InvalidationLoggerTest, TestCallbacks) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;

  log.RegisterObserver(&observer_test);
  log.OnStateChange(syncer::INVALIDATIONS_ENABLED);
  EXPECT_TRUE(observer_test.state_received);
  EXPECT_FALSE(observer_test.update_id_received);
  EXPECT_FALSE(observer_test.registration_change_received);
  EXPECT_FALSE(observer_test.invalidation_received);
  EXPECT_FALSE(observer_test.debug_message_received);
  EXPECT_FALSE(observer_test.detailed_status_received);

  observer_test.ResetStates();

  log.OnInvalidation(syncer::ObjectIdInvalidationMap());
  EXPECT_TRUE(observer_test.invalidation_received);
  EXPECT_FALSE(observer_test.state_received);
  EXPECT_FALSE(observer_test.update_id_received);
  EXPECT_FALSE(observer_test.registration_change_received);
  EXPECT_FALSE(observer_test.debug_message_received);
  EXPECT_FALSE(observer_test.detailed_status_received);

  log.UnregisterObserver(&observer_test);
}

// Test that after registering an observer and then unregistering it
// no callbacks regarding that observer are called.
// (i.e. the observer is cleanly removed)
TEST(InvalidationLoggerTest, TestReleaseOfObserver) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;

  log.RegisterObserver(&observer_test);
  log.UnregisterObserver(&observer_test);

  log.OnInvalidation(syncer::ObjectIdInvalidationMap());
  log.OnStateChange(syncer::INVALIDATIONS_ENABLED);
  log.OnRegistration(std::string());
  log.OnUnregistration(std::string());
  log.OnDebugMessage(base::DictionaryValue());
  log.OnUpdateIds(std::map<std::string, syncer::ObjectIdSet>());
  EXPECT_FALSE(observer_test.registration_change_received);
  EXPECT_FALSE(observer_test.update_id_received);
  EXPECT_FALSE(observer_test.invalidation_received);
  EXPECT_FALSE(observer_test.state_received);
  EXPECT_FALSE(observer_test.debug_message_received);
  EXPECT_FALSE(observer_test.detailed_status_received);
}

// Test the EmitContet in InvalidationLogger is actually
// sending state and updateIds notifications.
TEST(InvalidationLoggerTest, TestEmitContent) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;

  log.RegisterObserver(&observer_test);
  EXPECT_FALSE(observer_test.state_received);
  EXPECT_FALSE(observer_test.update_id_received);
  log.EmitContent();
  // Expect state and registered handlers only because no Ids were registered.
  EXPECT_TRUE(observer_test.state_received);
  EXPECT_TRUE(observer_test.registration_change_received);
  EXPECT_FALSE(observer_test.update_id_received);
  EXPECT_FALSE(observer_test.invalidation_received);
  EXPECT_FALSE(observer_test.debug_message_received);
  EXPECT_FALSE(observer_test.detailed_status_received);

  observer_test.ResetStates();
  std::map<std::string, syncer::ObjectIdSet> test_map;
  test_map["Test"] = syncer::ObjectIdSet();
  log.OnUpdateIds(test_map);
  EXPECT_TRUE(observer_test.update_id_received);
  observer_test.ResetStates();

  log.EmitContent();
  // Expect now state, ids and registered handlers change.
  EXPECT_TRUE(observer_test.state_received);
  EXPECT_TRUE(observer_test.update_id_received);
  EXPECT_TRUE(observer_test.registration_change_received);
  EXPECT_FALSE(observer_test.invalidation_received);
  EXPECT_FALSE(observer_test.debug_message_received);
  EXPECT_FALSE(observer_test.detailed_status_received);
  log.UnregisterObserver(&observer_test);
}

// Test that the updateId notification actually sends the same ObjectId that
// was sent to the Observer.
// The ObserverTest rebuilds the map that was sent in pieces by the logger.
TEST(InvalidationLoggerTest, TestUpdateIdsMap) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;
  std::map<std::string, syncer::ObjectIdSet> send_test_map;
  std::map<std::string, syncer::ObjectIdCountMap> expected_received_map;
  log.RegisterObserver(&observer_test);

  syncer::ObjectIdSet sync_set_A;
  syncer::ObjectIdCountMap counted_sync_set_A;

  ObjectId o1(1000, "DataType1");
  sync_set_A.insert(o1);
  counted_sync_set_A[o1] = 0;

  ObjectId o2(1000, "DataType2");
  sync_set_A.insert(o2);
  counted_sync_set_A[o2] = 0;

  syncer::ObjectIdSet sync_set_B;
  syncer::ObjectIdCountMap counted_sync_set_B;

  ObjectId o3(1020, "DataTypeA");
  sync_set_B.insert(o3);
  counted_sync_set_B[o3] = 0;

  send_test_map["TestA"] = sync_set_A;
  send_test_map["TestB"] = sync_set_B;
  expected_received_map["TestA"] = counted_sync_set_A;
  expected_received_map["TestB"] = counted_sync_set_B;

  // Send the objects ids registered for the two different handler name.
  log.OnUpdateIds(send_test_map);
  EXPECT_EQ(expected_received_map, observer_test.update_id_replicated);

  syncer::ObjectIdSet sync_set_B2;
  syncer::ObjectIdCountMap counted_sync_set_B2;

  ObjectId o4(1020, "DataTypeF");
  sync_set_B2.insert(o4);
  counted_sync_set_B2[o4] = 0;

  ObjectId o5(1020, "DataTypeG");
  sync_set_B2.insert(o5);
  counted_sync_set_B2[o5] = 0;

  send_test_map["TestB"] = sync_set_B2;
  expected_received_map["TestB"] = counted_sync_set_B2;

  // Test now that if we replace the registered datatypes for TestB, the
  // original don't show up again.
  log.OnUpdateIds(send_test_map);
  EXPECT_EQ(expected_received_map, observer_test.update_id_replicated);

  // The emit content should return the same map too.
  observer_test.ResetStates();
  log.EmitContent();
  EXPECT_EQ(expected_received_map, observer_test.update_id_replicated);
  log.UnregisterObserver(&observer_test);
}

// Test that the invalidation notification changes the total count
// of invalidations received for that datatype.
TEST(InvalidationLoggerTest, TestInvalidtionsTotalCount) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;
  log.RegisterObserver(&observer_test);

  std::map<std::string, syncer::ObjectIdSet> send_test_map;
  std::map<std::string, syncer::ObjectIdCountMap> expected_received_map;
  syncer::ObjectIdSet sync_set;
  syncer::ObjectIdCountMap counted_sync_set;

  ObjectId o1(1020, "DataTypeA");
  sync_set.insert(o1);
  counted_sync_set[o1] = 1;

  // Generate invalidation for datatype A only.
  syncer::ObjectIdInvalidationMap fake_invalidations =
      syncer::ObjectIdInvalidationMap::InvalidateAll(sync_set);

  ObjectId o2(1040, "DataTypeB");
  sync_set.insert(o2);
  counted_sync_set[o2] = 0;

  // Registed the two objectIds and send an invalidation only for the
  // Datatype A.
  send_test_map["Test"] = sync_set;
  log.OnUpdateIds(send_test_map);
  log.OnInvalidation(fake_invalidations);

  expected_received_map["Test"] = counted_sync_set;

  // Reset the state of the observer to receive the ObjectIds with the
  // count of invalidations received (1 and 0).
  observer_test.ResetStates();
  log.EmitContent();
  EXPECT_EQ(expected_received_map, observer_test.update_id_replicated);

  log.UnregisterObserver(&observer_test);
}

// Test that registered handlers are being sent to the observers.
TEST(InvalidationLoggerTest, TestRegisteredHandlers) {
  InvalidationLogger log;
  InvalidationLoggerObserverTest observer_test;
  log.RegisterObserver(&observer_test);

  log.OnRegistration(std::string("FakeHandler1"));
  std::multiset<std::string> test_multiset;
  test_multiset.insert("FakeHandler1");
  EXPECT_TRUE(observer_test.registration_change_received);
  EXPECT_EQ(observer_test.registered_handlers, test_multiset);

  observer_test.ResetStates();
  log.OnRegistration(std::string("FakeHandler2"));
  test_multiset.insert("FakeHandler2");
  EXPECT_TRUE(observer_test.registration_change_received);
  EXPECT_EQ(observer_test.registered_handlers, test_multiset);

  observer_test.ResetStates();
  log.OnUnregistration(std::string("FakeHandler2"));
  test_multiset.erase("FakeHandler2");
  EXPECT_TRUE(observer_test.registration_change_received);
  EXPECT_EQ(observer_test.registered_handlers, test_multiset);

  log.UnregisterObserver(&observer_test);
}
}  // namespace invalidation
