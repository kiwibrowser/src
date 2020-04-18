// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sessions_helper.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_integration_test_util.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/sync/user_events/user_event_service.h"
#include "components/variations/variations_associated_data.h"

using fake_server::FakeServer;
using sync_pb::UserEventSpecifics;
using sync_pb::SyncEntity;
using sync_pb::CommitResponse;

namespace {

UserEventSpecifics CreateTestEvent(int microseconds) {
  UserEventSpecifics specifics;
  specifics.set_event_time_usec(microseconds);
  specifics.set_navigation_id(microseconds);
  specifics.mutable_test_event();
  return specifics;
}

std::string GetAccountId() {
#if defined(OS_CHROMEOS)
  // TODO(vitaliii): Unify the two, because it takes ages to debug and
  // impossible to discover otherwise.
  return "user@gmail.com";
#else
  return "gaia-id-user@gmail.com";
#endif
}

UserEventSpecifics CreateUserConsent(int microseconds) {
  UserEventSpecifics specifics;
  specifics.set_event_time_usec(microseconds);
  specifics.mutable_user_consent()->set_account_id(GetAccountId());
  return specifics;
}

CommitResponse::ResponseType BounceType(
    CommitResponse::ResponseType type,
    const syncer::LoopbackServerEntity& entity) {
  return type;
}

CommitResponse::ResponseType TransientErrorFirst(
    bool* first,
    UserEventSpecifics* retry_specifics,
    const syncer::LoopbackServerEntity& entity) {
  if (*first) {
    *first = false;
    SyncEntity sync_entity;
    entity.SerializeAsProto(&sync_entity);
    *retry_specifics = sync_entity.specifics().user_event();
    return CommitResponse::TRANSIENT_ERROR;
  } else {
    return CommitResponse::SUCCESS;
  }
}

class UserEventEqualityChecker : public SingleClientStatusChangeChecker {
 public:
  UserEventEqualityChecker(browser_sync::ProfileSyncService* service,
                           FakeServer* fake_server,
                           std::vector<UserEventSpecifics> expected_specifics)
      : SingleClientStatusChangeChecker(service), fake_server_(fake_server) {
    for (const UserEventSpecifics& specifics : expected_specifics) {
      expected_specifics_.insert(std::pair<int64_t, UserEventSpecifics>(
          specifics.event_time_usec(), specifics));
    }
  }

  bool IsExitConditionSatisfied() override {
    std::vector<SyncEntity> entities =
        fake_server_->GetSyncEntitiesByModelType(syncer::USER_EVENTS);

    // |entities.size()| is only going to grow, if |entities.size()| ever
    // becomes bigger then all hope is lost of passing, stop now.
    EXPECT_GE(expected_specifics_.size(), entities.size());

    if (expected_specifics_.size() > entities.size()) {
      return false;
    }

    // Number of events on server matches expected, exit condition is satisfied.
    // Let's verify that content matches as well. It is safe to modify
    // |expected_specifics_|.
    for (const SyncEntity& entity : entities) {
      UserEventSpecifics server_specifics = entity.specifics().user_event();
      auto iter = expected_specifics_.find(server_specifics.event_time_usec());
      // We don't expect to encounter id matching events with different values,
      // this isn't going to recover so fail the test case now.
      EXPECT_TRUE(expected_specifics_.end() != iter);
      // TODO(skym): This may need to change if we start updating navigation_id
      // based on what sessions data is committed, and end up committing the
      // same event multiple times.
      EXPECT_EQ(iter->second.navigation_id(), server_specifics.navigation_id());
      EXPECT_EQ(iter->second.event_case(), server_specifics.event_case());

      expected_specifics_.erase(iter);
    }

    return true;
  }

  std::string GetDebugMessage() const override {
    return "Waiting server side USER_EVENTS to match expected.";
  }

 private:
  FakeServer* fake_server_;
  std::multimap<int64_t, UserEventSpecifics> expected_specifics_;
};

// A more simplistic version of UserEventEqualityChecker that only checks the
// case of the events. This is helpful if you do not know (or control) some of
// the fields of the events that are created.
class UserEventCaseChecker : public SingleClientStatusChangeChecker {
 public:
  UserEventCaseChecker(
      browser_sync::ProfileSyncService* service,
      FakeServer* fake_server,
      std::multiset<UserEventSpecifics::EventCase> expected_cases)
      : SingleClientStatusChangeChecker(service),
        fake_server_(fake_server),
        expected_cases_(expected_cases) {}

  bool IsExitConditionSatisfied() override {
    std::vector<SyncEntity> entities =
        fake_server_->GetSyncEntitiesByModelType(syncer::USER_EVENTS);

    // |entities.size()| is only going to grow, if |entities.size()| ever
    // becomes bigger then all hope is lost of passing, stop now.
    EXPECT_GE(expected_cases_.size(), entities.size());

    if (expected_cases_.size() > entities.size()) {
      return false;
    }

    // Number of events on server matches expected, exit condition is satisfied.
    // Let's verify that content matches as well. It is safe to modify
    // |expected_specifics_|.
    for (const SyncEntity& entity : entities) {
      UserEventSpecifics::EventCase actual =
          entity.specifics().user_event().event_case();
      auto iter = expected_cases_.find(actual);
      if (iter != expected_cases_.end()) {
        expected_cases_.erase(iter);
      } else {
        ADD_FAILURE() << actual;
      }
    }

    return true;
  }

  std::string GetDebugMessage() const override {
    return "Waiting server side USER_EVENTS to match expected.";
  }

 private:
  FakeServer* fake_server_;
  std::multiset<UserEventSpecifics::EventCase> expected_cases_;
};

class SingleClientUserEventsSyncTest : public SyncTest {
 public:
  SingleClientUserEventsSyncTest() : SyncTest(SINGLE_CLIENT) {
    DisableVerifier();
  }

  bool ExpectUserEvents(std::vector<UserEventSpecifics> expected_specifics) {
    return UserEventEqualityChecker(GetSyncService(0), GetFakeServer(),
                                    expected_specifics)
        .Wait();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientUserEventsSyncTest);
};

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, Sanity) {
  ASSERT_TRUE(SetupSync());
  EXPECT_EQ(
      0u,
      GetFakeServer()->GetSyncEntitiesByModelType(syncer::USER_EVENTS).size());
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));
  const UserEventSpecifics specifics = CreateTestEvent(0);
  event_service->RecordUserEvent(specifics);
  EXPECT_TRUE(ExpectUserEvents({specifics}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, RetrySequential) {
  ASSERT_TRUE(SetupSync());
  const UserEventSpecifics specifics1 = CreateTestEvent(1);
  const UserEventSpecifics specifics2 = CreateTestEvent(2);
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));

  GetFakeServer()->OverrideResponseType(
      base::Bind(&BounceType, CommitResponse::TRANSIENT_ERROR));
  event_service->RecordUserEvent(specifics1);

  // This will block until we hit a TRANSIENT_ERROR, at which point we will
  // regain control and can switch back to SUCCESS.
  EXPECT_TRUE(ExpectUserEvents({specifics1}));
  GetFakeServer()->OverrideResponseType(
      base::Bind(&BounceType, CommitResponse::SUCCESS));
  // Because the fake server records commits even on failure, we are able to
  // verify that the commit for this event reached the server twice.
  EXPECT_TRUE(ExpectUserEvents({specifics1, specifics1}));

  // Only record |specifics2| after |specifics1| was successful to avoid race
  // conditions.
  event_service->RecordUserEvent(specifics2);
  EXPECT_TRUE(ExpectUserEvents({specifics1, specifics1, specifics2}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, RetryParallel) {
  ASSERT_TRUE(SetupSync());
  bool first = true;
  const UserEventSpecifics specifics1 = CreateTestEvent(1);
  const UserEventSpecifics specifics2 = CreateTestEvent(2);
  UserEventSpecifics retry_specifics;

  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));

  // We're not really sure if |specifics1| or |specifics2| is going to see the
  // error, so record the one that does into |retry_specifics| and use it in
  // expectations.
  GetFakeServer()->OverrideResponseType(
      base::Bind(&TransientErrorFirst, &first, &retry_specifics));

  event_service->RecordUserEvent(specifics2);
  event_service->RecordUserEvent(specifics1);
  EXPECT_TRUE(ExpectUserEvents({specifics1, specifics2}));
  EXPECT_TRUE(ExpectUserEvents({specifics1, specifics2, retry_specifics}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, NoHistory) {
  const UserEventSpecifics testEvent1 = CreateTestEvent(1);
  const UserEventSpecifics testEvent2 = CreateTestEvent(2);
  const UserEventSpecifics testEvent3 = CreateTestEvent(3);
  const UserEventSpecifics consent1 = CreateUserConsent(4);
  const UserEventSpecifics consent2 = CreateUserConsent(5);

  ASSERT_TRUE(SetupSync());
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));

  event_service->RecordUserEvent(testEvent1);
  event_service->RecordUserEvent(consent1);

  // Wait until the first two events are committed before disabling sync,
  // because disabled TYPED_URLS also disables user event sync, dropping all
  // uncommitted consents.
  EXPECT_TRUE(ExpectUserEvents({testEvent1, consent1}));
  ASSERT_TRUE(GetClient(0)->DisableSyncForDatatype(syncer::TYPED_URLS));

  event_service->RecordUserEvent(testEvent2);
  event_service->RecordUserEvent(consent2);
  ASSERT_TRUE(GetClient(0)->EnableSyncForDatatype(syncer::TYPED_URLS));
  event_service->RecordUserEvent(testEvent3);

  // No |testEvent2| because it was recorded while history was disabled.
  EXPECT_TRUE(ExpectUserEvents({testEvent1, consent1, consent2, testEvent3}));
}

// Test that events that are logged before sync is enabled don't get lost.
IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, LoggedBeforeSyncSetup) {
  const UserEventSpecifics consent1 = CreateUserConsent(1);
  const UserEventSpecifics consent2 = CreateUserConsent(2);
  ASSERT_TRUE(SetupClients());
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));
  event_service->RecordUserEvent(consent1);
  EXPECT_TRUE(ExpectUserEvents({}));
  ASSERT_TRUE(SetupSync());
  EXPECT_TRUE(ExpectUserEvents({consent1}));
  event_service->RecordUserEvent(consent2);
  EXPECT_TRUE(ExpectUserEvents({consent1, consent2}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, NoSessions) {
  const UserEventSpecifics specifics = CreateTestEvent(1);
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(GetClient(0)->DisableSyncForDatatype(syncer::PROXY_TABS));
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));

  event_service->RecordUserEvent(specifics);

  // PROXY_TABS shouldn't affect us in any way.
  EXPECT_TRUE(ExpectUserEvents({specifics}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, Encryption) {
  const UserEventSpecifics testEvent1 = CreateTestEvent(1);
  const UserEventSpecifics testEvent2 = CreateTestEvent(2);
  const UserEventSpecifics consent1 = CreateUserConsent(3);
  const UserEventSpecifics consent2 = CreateUserConsent(4);

  ASSERT_TRUE(SetupSync());
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));
  event_service->RecordUserEvent(testEvent1);
  event_service->RecordUserEvent(consent1);
  ASSERT_TRUE(EnableEncryption(0));
  EXPECT_TRUE(ExpectUserEvents({testEvent1, consent1}));
  event_service->RecordUserEvent(testEvent2);
  event_service->RecordUserEvent(consent2);

  // Just checking that we don't see testEvent2 isn't very convincing yet,
  // because it may simply not have reached the server yet. So lets send
  // something else through the system that we can wait on before checking.
  // Tab/SESSIONS data was picked fairly arbitrarily, note that we expect 2
  // entries, one for the window/header and one for the tab.
  sessions_helper::OpenTab(0, GURL("http://www.one.com/"));
  EXPECT_TRUE(ServerCountMatchStatusChecker(syncer::SESSIONS, 2).Wait());
  EXPECT_TRUE(ExpectUserEvents({testEvent1, consent1, consent2}));
}

IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest, FieldTrial) {
  const std::string trial_name = "TrialName";
  const std::string group_name = "GroupName";

  ASSERT_TRUE(SetupSync());
  variations::AssociateGoogleVariationID(variations::CHROME_SYNC_EVENT_LOGGER,
                                         trial_name, group_name, 123);
  base::FieldTrialList::CreateFieldTrial(trial_name, group_name);
  base::FieldTrialList::FindFullName(trial_name);

  UserEventCaseChecker(GetSyncService(0), GetFakeServer(),
                       {UserEventSpecifics::kFieldTrialEvent})
      .Wait();
}

// TODO(jkrcal): Reenable the test after more investigation.
// https://crbug.com/843847
IN_PROC_BROWSER_TEST_F(SingleClientUserEventsSyncTest,
                       DISABLED_PreserveConsentsOnDisableSync) {
  const UserEventSpecifics testEvent1 = CreateTestEvent(1);
  const UserEventSpecifics consent1 = CreateUserConsent(2);

  ASSERT_TRUE(SetupSync());
  syncer::UserEventService* event_service =
      browser_sync::UserEventServiceFactory::GetForProfile(GetProfile(0));
  event_service->RecordUserEvent(testEvent1);
  event_service->RecordUserEvent(consent1);

  GetClient(0)->StopSyncService(syncer::SyncService::CLEAR_DATA);
  ASSERT_TRUE(GetClient(0)->StartSyncService());

  EXPECT_TRUE(ExpectUserEvents({consent1}));
}

}  // namespace
