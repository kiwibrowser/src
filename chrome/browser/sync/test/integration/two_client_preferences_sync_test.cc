// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/guid.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/sync/test/integration/preferences_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_integration_test_util.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

using preferences_helper::BooleanPrefMatches;
using preferences_helper::ChangeBooleanPref;
using preferences_helper::ChangeIntegerPref;
using preferences_helper::ChangeListPref;
using preferences_helper::ChangeStringPref;
using preferences_helper::GetPrefs;

class TwoClientPreferencesSyncTest : public SyncTest {
 public:
  TwoClientPreferencesSyncTest() : SyncTest(TWO_CLIENT) {}
  ~TwoClientPreferencesSyncTest() override {}

  bool TestUsesSelfNotifications() override { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientPreferencesSyncTest);
};

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, E2E_ONLY(Sanity)) {
  DisableVerifier();
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  const std::string new_home_page = base::StringPrintf(
      "https://example.com/%s", base::GenerateGUID().c_str());
  ChangeStringPref(0, prefs::kHomePage, new_home_page);
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  for (int i = 0; i < num_clients(); ++i) {
    ASSERT_EQ(new_home_page, GetPrefs(i)->GetString(prefs::kHomePage));
  }
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, E2E_ENABLED(BooleanPref)) {
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       E2E_ENABLED(Bidirectional)) {
  ASSERT_TRUE(SetupSync());

  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());

  ChangeStringPref(0, prefs::kHomePage, "http://www.google.com/0");
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  EXPECT_EQ("http://www.google.com/0",
            GetPrefs(0)->GetString(prefs::kHomePage));

  ChangeStringPref(1, prefs::kHomePage, "http://www.google.com/1");
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  EXPECT_EQ("http://www.google.com/1",
            GetPrefs(0)->GetString(prefs::kHomePage));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       E2E_ENABLED(UnsyncableBooleanPref)) {
  ASSERT_TRUE(SetupSync());
  DisableVerifier();
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kDisableScreenshots).Wait());

  // This pref is not syncable.
  ChangeBooleanPref(0, prefs::kDisableScreenshots);

  // This pref is syncable.
  ChangeStringPref(0, prefs::kHomePage, "http://news.google.com");

  // Wait until the syncable pref is synced, then expect that the non-syncable
  // one is still out of sync.
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  ASSERT_FALSE(BooleanPrefMatches(prefs::kDisableScreenshots));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, E2E_ENABLED(StringPref)) {
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());

  ChangeStringPref(0, prefs::kHomePage, "http://news.google.com");
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       E2E_ENABLED(ComplexPrefs)) {
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(IntegerPrefMatchChecker(prefs::kRestoreOnStartup).Wait());
  ASSERT_TRUE(ListPrefMatchChecker(prefs::kURLsToRestoreOnStartup).Wait());

  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 0);
  ASSERT_TRUE(IntegerPrefMatchChecker(prefs::kRestoreOnStartup).Wait());

  base::ListValue urls;
  urls.AppendString("http://www.google.com/");
  urls.AppendString("http://www.flickr.com/");
  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 4);
  ChangeListPref(0, prefs::kURLsToRestoreOnStartup, urls);
  ASSERT_TRUE(IntegerPrefMatchChecker(prefs::kRestoreOnStartup).Wait());
  ASSERT_TRUE(ListPrefMatchChecker(prefs::kURLsToRestoreOnStartup).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       E2E_ENABLED(SingleClientEnabledEncryptionBothChanged)) {
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());

  ASSERT_TRUE(EnableEncryption(0));
  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ChangeStringPref(1, prefs::kHomePage, "http://www.google.com/1");
  ASSERT_TRUE(AwaitEncryptionComplete(0));
  ASSERT_TRUE(AwaitEncryptionComplete(1));
  ASSERT_TRUE(StringPrefMatchChecker(prefs::kHomePage).Wait());
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
      E2E_ENABLED(BothClientsEnabledEncryptionAndChangedMultipleTimes)) {
  ASSERT_TRUE(SetupSync());
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(EnableEncryption(1));
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kHomePageIsNewTabPage).Wait());

  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kShowHomeButton).Wait());
  ChangeBooleanPref(0, prefs::kShowHomeButton);
  ASSERT_TRUE(BooleanPrefMatchChecker(prefs::kShowHomeButton).Wait());
}
