// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/sync/test/integration/autofill_helper.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/performance/sync_timing_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"

using autofill::ServerFieldType;
using autofill::AutofillKey;
using autofill::AutofillProfile;
using autofill_helper::GetAllAutoFillProfiles;
using autofill_helper::GetAllKeys;
using autofill_helper::GetKeyCount;
using autofill_helper::GetProfileCount;
using autofill_helper::RemoveKeys;
using autofill_helper::SetProfiles;
using sync_timing_helper::PrintResult;
using sync_timing_helper::TimeMutualSyncCycle;

// See comments in typed_urls_sync_perf_test.cc for reasons for these
// magic numbers.
//
// TODO(akalin): If this works, decomp the magic number calculation
// into a macro and have all the perf tests use it.
static const int kNumKeys = 163;
static const int kNumProfiles = 163;

class AutofillSyncPerfTest : public SyncTest {
 public:
  AutofillSyncPerfTest()
      : SyncTest(TWO_CLIENT),
        guid_number_(0),
        name_number_(0),
        value_number_(0) {}

  // Adds |num_profiles| new autofill profiles to the sync profile |profile|.
  void AddProfiles(int profile, int num_profiles);

  // Updates all autofill profiles for the sync profile |profile|.
  void UpdateProfiles(int profile);

  // Removes all autofill profiles from |profile|.
  void RemoveProfiles(int profile);

  // Adds |num_keys| new autofill keys to the sync profile |profile|.
  void AddKeys(int profile, int num_keys);

 private:
  // Returns a new unique autofill profile.
  const AutofillProfile NextAutofillProfile();

  // Returns a new unique autofill key.
  const AutofillKey NextAutofillKey();

  // Returns an unused unique guid.
  const std::string NextGUID();

  // Returns a unique guid based on the input integer |n|.
  const std::string IntToGUID(int n);

  // Returns a new unused unique name.
  const std::string NextName();

  // Returns a unique name based on the input integer |n|.
  const std::string IntToName(int n);

  // Returns a new unused unique value for autofill entries.
  const std::string NextValue();

  // Returnes a unique value based on the input integer |n|.
  const std::string IntToValue(int n);

  int guid_number_;
  int name_number_;
  int value_number_;
  DISALLOW_COPY_AND_ASSIGN(AutofillSyncPerfTest);
};

void AutofillSyncPerfTest::AddProfiles(int profile, int num_profiles) {
  const std::vector<AutofillProfile*>& all_profiles =
      GetAllAutoFillProfiles(profile);
  std::vector<AutofillProfile> autofill_profiles;
  for (size_t i = 0; i < all_profiles.size(); ++i) {
    autofill_profiles.push_back(*all_profiles[i]);
  }
  for (int i = 0; i < num_profiles; ++i) {
    autofill_profiles.push_back(NextAutofillProfile());
  }
  SetProfiles(profile, &autofill_profiles);
}

void AutofillSyncPerfTest::UpdateProfiles(int profile) {
  const std::vector<AutofillProfile*>& all_profiles =
      GetAllAutoFillProfiles(profile);
  std::vector<AutofillProfile> autofill_profiles;
  for (size_t i = 0; i < all_profiles.size(); ++i) {
    autofill_profiles.push_back(*all_profiles[i]);
    autofill_profiles.back().SetRawInfo(autofill::NAME_FIRST,
                                        base::UTF8ToUTF16(NextName()));
  }
  SetProfiles(profile, &autofill_profiles);
}

void AutofillSyncPerfTest::RemoveProfiles(int profile) {
  std::vector<AutofillProfile> empty;
  SetProfiles(profile, &empty);
}

void AutofillSyncPerfTest::AddKeys(int profile, int num_keys) {
  std::set<AutofillKey> keys;
  for (int i = 0; i < num_keys; ++i) {
    keys.insert(NextAutofillKey());
  }
  autofill_helper::AddKeys(profile, keys);
}

const AutofillProfile AutofillSyncPerfTest::NextAutofillProfile() {
  AutofillProfile profile;
  autofill::test::SetProfileInfoWithGuid(&profile, NextGUID().c_str(),
                                         NextName().c_str(), "", "", "", "", "",
                                         "", "", "", "", "", "");
  return profile;
}

const AutofillKey AutofillSyncPerfTest::NextAutofillKey() {
  return AutofillKey(NextName().c_str(), NextName().c_str());
}

const std::string AutofillSyncPerfTest::NextGUID() {
  return IntToGUID(guid_number_++);
}

const std::string AutofillSyncPerfTest::IntToGUID(int n) {
  return base::StringPrintf("00000000-0000-0000-0000-%012X", n);
}

const std::string AutofillSyncPerfTest::NextName() {
  return IntToName(name_number_++);
}

const std::string AutofillSyncPerfTest::IntToName(int n) {
  return base::StringPrintf("Name%d", n);
}

const std::string AutofillSyncPerfTest::NextValue() {
  return IntToValue(value_number_++);
}

const std::string AutofillSyncPerfTest::IntToValue(int n) {
  return base::StringPrintf("Value%d", n);
}

void ForceSync(int profile) {
  static int id = 0;
  ++id;
  EXPECT_TRUE(bookmarks_helper::AddURL(
                  profile, 0, bookmarks_helper::IndexedURLTitle(id),
                  GURL(bookmarks_helper::IndexedURL(id))) != nullptr);
}

IN_PROC_BROWSER_TEST_F(AutofillSyncPerfTest, AutofillProfiles_P0) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfiles(0, kNumProfiles);
  base::TimeDelta dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumProfiles, GetProfileCount(1));
  PrintResult("autofill", "add_autofill_profiles", dt);

  UpdateProfiles(0);
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumProfiles, GetProfileCount(1));
  PrintResult("autofill", "update_autofill_profiles", dt);

  RemoveProfiles(0);
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(0, GetProfileCount(1));
  PrintResult("autofill", "delete_autofill_profiles", dt);
}

IN_PROC_BROWSER_TEST_F(AutofillSyncPerfTest, Autofill_P0) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddKeys(0, kNumKeys);
  // TODO(lipalani): fix this. The following line is added to force sync.
  ForceSync(0);
  base::TimeDelta dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumKeys, GetKeyCount(1));
  PrintResult("autofill", "add_autofill_keys", dt);

  RemoveKeys(0);
  // TODO(lipalani): fix this. The following line is added to force sync.
  ForceSync(0);
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(0, GetKeyCount(1));
  PrintResult("autofill", "delete_autofill_keys", dt);
}
