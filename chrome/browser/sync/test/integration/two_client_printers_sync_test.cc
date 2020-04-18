// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "base/macros.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/sync/test/integration/printers_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "content/public/test/test_utils.h"

namespace {

using printers_helper::AddPrinter;
using printers_helper::AllProfilesContainSamePrinters;
using printers_helper::EditPrinterDescription;
using printers_helper::CreateTestPrinter;
using printers_helper::GetPrinterCount;
using printers_helper::GetPrinterStore;
using printers_helper::PrintersMatchChecker;
using printers_helper::RemovePrinter;

constexpr char kOverwrittenDescription[] = "I should not show up";
constexpr char kLatestDescription[] = "YAY!  More recent changes win!";

class TwoClientPrintersSyncTest : public SyncTest {
 public:
  TwoClientPrintersSyncTest() : SyncTest(TWO_CLIENT) {}
  ~TwoClientPrintersSyncTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientPrintersSyncTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, NoPrinters) {
  ASSERT_TRUE(SetupSync());

  ASSERT_TRUE(PrintersMatchChecker().Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, OnePrinter) {
  ASSERT_TRUE(SetupSync());

  AddPrinter(GetPrinterStore(0), CreateTestPrinter(2));

  ASSERT_TRUE(PrintersMatchChecker().Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, SimultaneousAdd) {
  ASSERT_TRUE(SetupSync());

  AddPrinter(GetPrinterStore(0), CreateTestPrinter(1));
  AddPrinter(GetPrinterStore(1), CreateTestPrinter(2));

  // Each store is guaranteed to have 1 printer because the tests run on the UI
  // thread.  ApplySyncChanges happens after we wait on the checker.
  ASSERT_EQ(1, GetPrinterCount(0));
  ASSERT_EQ(1, GetPrinterCount(1));

  ASSERT_TRUE(PrintersMatchChecker().Wait());
  EXPECT_EQ(2, GetPrinterCount(0));
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, RemovePrinter) {
  ASSERT_TRUE(SetupSync());

  AddPrinter(GetPrinterStore(0), CreateTestPrinter(1));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(2));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(3));

  // Verify the profiles have the same printers
  ASSERT_TRUE(PrintersMatchChecker().Wait());
  EXPECT_EQ(3, GetPrinterCount(0));

  // Remove printer 2 from store 1
  RemovePrinter(GetPrinterStore(1), 2);

  ASSERT_TRUE(PrintersMatchChecker().Wait());
  EXPECT_EQ(2, GetPrinterCount(0));
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, RemoveAndEditPrinters) {
  const std::string updated_description = "Testing changes";

  ASSERT_TRUE(SetupSync());

  AddPrinter(GetPrinterStore(0), CreateTestPrinter(1));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(2));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(3));

  // Verify the profiles have the same printers
  ASSERT_TRUE(PrintersMatchChecker().Wait());
  EXPECT_EQ(3, GetPrinterCount(0));

  // Edit printer 1 from store 0
  ASSERT_TRUE(
      EditPrinterDescription(GetPrinterStore(0), 1, updated_description));

  // Remove printer 2 from store 1
  RemovePrinter(GetPrinterStore(1), 2);

  ASSERT_TRUE(PrintersMatchChecker().Wait());
  EXPECT_EQ(2, GetPrinterCount(0));
  EXPECT_EQ(updated_description,
            GetPrinterStore(1)->GetConfiguredPrinters()[0].description());
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, ConflictResolution) {
  ASSERT_TRUE(SetupSync());

  AddPrinter(GetPrinterStore(0), CreateTestPrinter(0));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(2));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(3));

  // Store 0 and 1 have 3 printers now.
  ASSERT_TRUE(PrintersMatchChecker().Wait());

  ASSERT_TRUE(
      EditPrinterDescription(GetPrinterStore(1), 0, kOverwrittenDescription));

  // Wait for a non-zero period (200ms).
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(200));

  ASSERT_TRUE(
      EditPrinterDescription(GetPrinterStore(0), 0, kLatestDescription));

  // Run tasks until the most recent update has been applied to all stores.
  AwaitMatchStatusChangeChecker wait_latest_description_propagated(
      base::BindRepeating([]() {
        return GetPrinterStore(0)->GetConfiguredPrinters()[0].description() ==
                   kLatestDescription &&
               GetPrinterStore(1)->GetConfiguredPrinters()[0].description() ==
                   kLatestDescription;
      }),
      "Description not propagated");
  ASSERT_TRUE(wait_latest_description_propagated.Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPrintersSyncTest, SimpleMerge) {
  ASSERT_TRUE(SetupClients());
  base::RunLoop().RunUntilIdle();

  // Store 0 has the even printers
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(0));
  AddPrinter(GetPrinterStore(0), CreateTestPrinter(2));

  // Store 1 has the odd printers
  AddPrinter(GetPrinterStore(1), CreateTestPrinter(1));
  AddPrinter(GetPrinterStore(1), CreateTestPrinter(3));

  ASSERT_TRUE(SetupSync());

  // Stores should contain the same values now.
  EXPECT_EQ(4, GetPrinterCount(0));
  EXPECT_TRUE(AllProfilesContainSamePrinters());
}
