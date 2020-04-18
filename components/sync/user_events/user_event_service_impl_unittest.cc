// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/user_event_service_impl.h"

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_feature_list.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/model/model_type_store_test_util.h"
#include "components/sync/model/recording_model_type_change_processor.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/user_events/user_event_sync_bridge.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ScopedFeatureList;
using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

std::unique_ptr<UserEventSpecifics> Event() {
  return std::make_unique<UserEventSpecifics>();
}

std::unique_ptr<UserEventSpecifics> AsTest(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_test_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> AsDetection(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_language_detection_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> AsTrial(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_field_trial_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> AsConsent(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_user_consent()->set_account_id("account_id");
  return specifics;
}

std::unique_ptr<UserEventSpecifics> WithNav(
    std::unique_ptr<UserEventSpecifics> specifics,
    int64_t navigation_id = 1) {
  specifics->set_navigation_id(navigation_id);
  return specifics;
}

// TODO(vitaliii): Merge this into FakeSyncService and use it instead.
class TestSyncService : public FakeSyncService {
 public:
  TestSyncService(bool is_engine_initialized,
                  bool is_using_secondary_passphrase,
                  ModelTypeSet preferred_data_types)
      : is_engine_initialized_(is_engine_initialized),
        is_using_secondary_passphrase_(is_using_secondary_passphrase),
        preferred_data_types_(preferred_data_types) {}

  bool IsEngineInitialized() const override { return is_engine_initialized_; }
  bool IsUsingSecondaryPassphrase() const override {
    return is_using_secondary_passphrase_;
  }

  ModelTypeSet GetPreferredDataTypes() const override {
    return preferred_data_types_;
  }

 private:
  bool is_engine_initialized_;
  bool is_using_secondary_passphrase_;
  ModelTypeSet preferred_data_types_;
};

class TestGlobalIdMapper : public GlobalIdMapper {
  void AddGlobalIdChangeObserver(GlobalIdChange callback) override {}
  int64_t GetLatestGlobalId(int64_t global_id) override { return global_id; }
};

class UserEventServiceImplTest : public testing::Test {
 protected:
  UserEventServiceImplTest()
      : field_trial_list_(nullptr),
        sync_service_(true, false, {HISTORY_DELETE_DIRECTIVES}) {}

  std::unique_ptr<UserEventSyncBridge> MakeBridge() {
    return std::make_unique<UserEventSyncBridge>(
        ModelTypeStoreTestUtil::FactoryForInMemoryStoreForTest(),
        RecordingModelTypeChangeProcessor::CreateProcessorAndAssignRawPointer(
            &processor_),
        &mapper_, &sync_service_);
  }

  TestSyncService* sync_service() { return &sync_service_; }
  const RecordingModelTypeChangeProcessor& processor() { return *processor_; }

 private:
  base::MessageLoop message_loop_;
  base::FieldTrialList field_trial_list_;
  TestSyncService sync_service_;
  RecordingModelTypeChangeProcessor* processor_;
  TestGlobalIdMapper mapper_;
};

TEST_F(UserEventServiceImplTest, MightRecordEventsFeatureEnabled) {
  // All conditions are met, might record.
  EXPECT_TRUE(UserEventServiceImpl::MightRecordEvents(false, sync_service()));
  // No sync service, will not record.
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(false, nullptr));
  // Off the record, will not record.
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(true, sync_service()));
}

TEST_F(UserEventServiceImplTest, MightRecordEventsFeatureDisabled) {
  // Will not record because the default on feature is overridden.
  ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(switches::kSyncUserEvents);
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(false, sync_service()));
}

TEST_F(UserEventServiceImplTest, ShouldRecord) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordNoHistory) {
  TestSyncService no_history_sync_service(true, false, ModelTypeSet());
  UserEventServiceImpl service(&no_history_sync_service, MakeBridge());

  // Only record events without navigation ids when history sync is off.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
}

TEST_F(UserEventServiceImplTest, ShouldRecordUserConsentNoHistory) {
  TestSyncService no_history_sync_service(true, false, ModelTypeSet());
  UserEventServiceImpl service(&no_history_sync_service, MakeBridge());
  service.RecordUserEvent(AsConsent(Event()));
  // UserConsent recording doesn't need history sync to be enabled.
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordPassphrase) {
  TestSyncService passphrase_sync_service(true, true,
                                          {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&passphrase_sync_service, MakeBridge());

  // Only record events without navigation ids when a passphrase is used.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordEngineOff) {
  TestSyncService engine_not_initialized_sync_service(
      false, false, {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&engine_not_initialized_sync_service,
                               MakeBridge());

  // Only record events without navigation ids when the engine is off.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordEmpty) {
  UserEventServiceImpl service(sync_service(), MakeBridge());

  // All untyped events should always be ignored.
  service.RecordUserEvent(Event());
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(Event()));
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordHasNavigationId) {
  UserEventServiceImpl service(sync_service(), MakeBridge());

  // Verify logic for types that might or might not have a navigation id.
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Verify logic for types that must have a navigation id.
  service.RecordUserEvent(AsDetection(Event()));
  EXPECT_EQ(2u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsDetection(Event())));
  EXPECT_EQ(3u, processor().put_multimap().size());

  // Verify logic for types that cannot have a navigation id.
  service.RecordUserEvent(AsTrial(Event()));
  EXPECT_EQ(4u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsTrial(Event())));
  EXPECT_EQ(4u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, SessionIdIsDifferent) {
  UserEventServiceImpl service1(sync_service(), MakeBridge());
  service1.RecordUserEvent(AsTest(Event()));
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put1 = processor().put_multimap().begin();
  int64_t session_id1 = put1->second->specifics.user_event().session_id();

  UserEventServiceImpl service2(sync_service(), MakeBridge());
  service2.RecordUserEvent(AsTest(Event()));
  // The object processor() points to has changed to be |service2|'s processor.
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put2 = processor().put_multimap().begin();
  int64_t session_id2 = put2->second->specifics.user_event().session_id();

  EXPECT_NE(session_id1, session_id2);
}

TEST_F(UserEventServiceImplTest, FieldTrial) {
  variations::AssociateGoogleVariationID(variations::CHROME_SYNC_EVENT_LOGGER,
                                         "trial", "group", 123);
  base::FieldTrialList::CreateFieldTrial("trial", "group");
  base::FieldTrialList::FindFullName("trial");

  UserEventServiceImpl service(sync_service(), MakeBridge());
  ASSERT_EQ(1u, processor().put_multimap().size());
  const UserEventSpecifics specifics =
      processor().put_multimap().begin()->second->specifics.user_event();
  ASSERT_EQ(1, specifics.field_trial_event().variation_ids_size());
  EXPECT_EQ(123u, specifics.field_trial_event().variation_ids(0));
}

}  // namespace

}  // namespace syncer
