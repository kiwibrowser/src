// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/retry_verifier.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "components/browser_sync/profile_sync_service.h"
#include "net/base/network_change_notifier.h"

namespace {

using bookmarks_helper::AddFolder;
using bookmarks_helper::ModelMatchesVerifier;
using syncer::SyncCycleSnapshot;

class SyncExponentialBackoffTest : public SyncTest {
 public:
  SyncExponentialBackoffTest() : SyncTest(SINGLE_CLIENT) {}
  ~SyncExponentialBackoffTest() override {}

  void SetUp() override {
    // This is needed to avoid spurious notifications initiated by the platform.
    net::NetworkChangeNotifier::SetTestNotificationsOnly(true);
    SyncTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncExponentialBackoffTest);
};

// Helper class that checks if a sync client has successfully gone through
// exponential backoff after it encounters an error.
class ExponentialBackoffChecker : public SingleClientStatusChangeChecker {
 public:
  explicit ExponentialBackoffChecker(browser_sync::ProfileSyncService* pss)
      : SingleClientStatusChangeChecker(pss) {
    const SyncCycleSnapshot& snap = service()->GetLastCycleSnapshot();
    retry_verifier_.Initialize(snap);
  }

  // Checks if backoff is complete. Called repeatedly each time PSS notifies
  // observers of a state change.
  bool IsExitConditionSatisfied() override {
    const SyncCycleSnapshot& snap = service()->GetLastCycleSnapshot();
    retry_verifier_.VerifyRetryInterval(snap);
    return (retry_verifier_.done() && retry_verifier_.Succeeded());
  }

  std::string GetDebugMessage() const override {
    return base::StringPrintf("Verifying backoff intervals (%d/%d)",
                              retry_verifier_.retry_count(),
                              RetryVerifier::kMaxRetry);
  }

 private:
  // Keeps track of the number of attempts at exponential backoff and its
  // related bookkeeping information for verification.
  RetryVerifier retry_verifier_;

  DISALLOW_COPY_AND_ASSIGN(ExponentialBackoffChecker);
};

IN_PROC_BROWSER_TEST_F(SyncExponentialBackoffTest, OfflineToOnline) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Add an item and ensure that sync is successful.
  ASSERT_TRUE(AddFolder(0, 0, "folder1"));
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());

  GetFakeServer()->DisableNetwork();

  // Add a new item to trigger another sync cycle.
  ASSERT_TRUE(AddFolder(0, 0, "folder2"));

  // Verify that the client goes into exponential backoff while it is unable to
  // reach the sync server.
  ASSERT_TRUE(ExponentialBackoffChecker(GetSyncService(0)).Wait());

  // Trigger network change notification and remember time when it happened.
  // Ensure that scheduler runs canary job immediately.
  GetFakeServer()->EnableNetwork();
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);

  base::Time network_notification_time = base::Time::Now();

  // Verify that sync was able to recover.
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  ASSERT_TRUE(ModelMatchesVerifier(0));

  // Verify that recovery time is short. Without canary job recovery time would
  // be more than 5 seconds.
  base::TimeDelta recovery_time =
      GetSyncService(0)->GetLastCycleSnapshot().sync_start_time() -
      network_notification_time;
  ASSERT_LE(recovery_time, base::TimeDelta::FromSeconds(2));
}

IN_PROC_BROWSER_TEST_F(SyncExponentialBackoffTest, TransientErrorTest) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Add an item and ensure that sync is successful.
  ASSERT_TRUE(AddFolder(0, 0, "folder1"));
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());

  GetFakeServer()->TriggerError(sync_pb::SyncEnums::TRANSIENT_ERROR);

  // Add a new item to trigger another sync cycle.
  ASSERT_TRUE(AddFolder(0, 0, "folder2"));

  // Verify that the client goes into exponential backoff while it is unable to
  // reach the sync server.
  ASSERT_TRUE(ExponentialBackoffChecker(GetSyncService(0)).Wait());
}

}  // namespace
