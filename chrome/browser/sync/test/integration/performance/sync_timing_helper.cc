// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/performance/sync_timing_helper.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_timing_helper {

base::TimeDelta TimeSyncCycle(ProfileSyncServiceHarness* client) {
  base::Time start = base::Time::Now();
  EXPECT_TRUE(UpdatedProgressMarkerChecker(client->service()).Wait());
  return base::Time::Now() - start;
}

base::TimeDelta TimeMutualSyncCycle(ProfileSyncServiceHarness* client,
                                    ProfileSyncServiceHarness* partner) {
  base::Time start = base::Time::Now();
  EXPECT_TRUE(client->AwaitMutualSyncCycleCompletion(partner));
  return base::Time::Now() - start;
}

base::TimeDelta TimeUntilQuiescence(
    const std::vector<ProfileSyncServiceHarness*>& clients) {
  base::Time start = base::Time::Now();
  EXPECT_TRUE(ProfileSyncServiceHarness::AwaitQuiescence(clients));
  return base::Time::Now() - start;
}

void PrintResult(const std::string& measurement,
                 const std::string& trace,
                 const base::TimeDelta& dt) {
  printf("*RESULT %s: %s= %s ms\n", measurement.c_str(), trace.c_str(),
         base::Int64ToString(dt.InMilliseconds()).c_str());
}

}  // namespace sync_timing_helper
