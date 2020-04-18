
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/sync/test/integration/autofill_helper.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"

using autofill::AutofillKey;
using autofill::AutofillTable;
using autofill::AutofillProfile;
using autofill::AutofillType;
using autofill::CreditCard;
using autofill::PersonalDataManager;
using autofill_helper::AddKeys;
using autofill_helper::AddProfile;
using autofill_helper::CreateAutofillProfile;
using autofill_helper::CreateUniqueAutofillProfile;
using autofill_helper::GetAllAutoFillProfiles;
using autofill_helper::GetAllKeys;
using autofill_helper::GetPersonalDataManager;
using autofill_helper::GetProfileCount;
using autofill_helper::KeysMatch;
using autofill_helper::ProfilesMatch;
using autofill_helper::PROFILE_FRASIER;
using autofill_helper::PROFILE_HOMER;
using autofill_helper::PROFILE_MARION;
using autofill_helper::PROFILE_NULL;
using autofill_helper::RemoveKey;
using autofill_helper::RemoveProfile;
using autofill_helper::SetCreditCards;
using autofill_helper::UpdateProfile;
using bookmarks_helper::AddFolder;
using bookmarks_helper::AddURL;
using bookmarks_helper::IndexedURL;
using bookmarks_helper::IndexedURLTitle;

class TwoClientAutofillSyncTest : public SyncTest {
 public:
  TwoClientAutofillSyncTest() : SyncTest(TWO_CLIENT) {}
  ~TwoClientAutofillSyncTest() override {}

  bool TestUsesSelfNotifications() override { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientAutofillSyncTest);
};

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, WebDataServiceSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a key.
  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0"));
  AddKeys(0, keys);
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllKeys(0).size());

  // Client1 adds a key.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-0"));
  AddKeys(1, keys);
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(2U, GetAllKeys(0).size());

  // Client0 adds a key with the same name.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-1"));
  AddKeys(0, keys);
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(3U, GetAllKeys(0).size());

  // Client1 removes a key.
  RemoveKey(1, AutofillKey("name1", "value1-0"));
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(2U, GetAllKeys(0).size());

  // Client0 removes the rest.
  RemoveKey(0, AutofillKey("name0", "value0"));
  RemoveKey(0, AutofillKey("name1", "value1-1"));
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(0U, GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddUnicodeProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey(base::WideToUTF16(L"Sigur R\u00F3s"),
                          base::WideToUTF16(L"\u00C1g\u00E6tis byrjun")));
  AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToSameProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0-0"));
  keys.insert(AutofillKey("name0", "value0-1"));
  keys.insert(AutofillKey("name1", "value1"));
  AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(2U, GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToDifferentProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys0;
  keys0.insert(AutofillKey("name0", "value0-0"));
  keys0.insert(AutofillKey("name1", "value1"));
  AddKeys(0, keys0);

  std::set<AutofillKey> keys1;
  keys1.insert(AutofillKey("name0", "value0-1"));
  keys1.insert(AutofillKey("name2", "value2"));
  keys1.insert(AutofillKey("name3", "value3"));
  AddKeys(1, keys1);

  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillKeysChecker(0, 1).Wait());
  ASSERT_EQ(5U, GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       PersonalDataManagerSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a profile.
  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  // Client1 adds a profile.
  AddProfile(1, CreateAutofillProfile(PROFILE_MARION));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(2U, GetAllAutoFillProfiles(0).size());

  // Client0 adds the same profile.
  AddProfile(0, CreateAutofillProfile(PROFILE_MARION));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(2U, GetAllAutoFillProfiles(0).size());

  // Client1 removes a profile.
  RemoveProfile(1, GetAllAutoFillProfiles(1)[0]->guid());
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  // Client0 updates a profile.
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                base::ASCIIToUTF16("Bart"));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  // Client1 removes remaining profile.
  RemoveProfile(1, GetAllAutoFillProfiles(1)[0]->guid());
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(0U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddDuplicateProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, SameProfileWithConflict) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AutofillProfile profile0 = CreateAutofillProfile(PROFILE_HOMER);
  AutofillProfile profile1 = CreateAutofillProfile(PROFILE_HOMER);
  profile1.SetRawInfo(autofill::PHONE_HOME_WHOLE_NUMBER,
                      base::ASCIIToUTF16("1234567890"));

  AddProfile(0, profile0);
  AddProfile(1, profile1);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddEmptyProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_NULL));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(0U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddMultipleProfiles) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(0, CreateAutofillProfile(PROFILE_MARION));
  AddProfile(0, CreateAutofillProfile(PROFILE_FRASIER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(3U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DeleteProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  RemoveProfile(1, GetAllAutoFillProfiles(1)[0]->guid());
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(0U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MergeProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  AddProfile(1, CreateAutofillProfile(PROFILE_MARION));
  AddProfile(1, CreateAutofillProfile(PROFILE_FRASIER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(3U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, UpdateFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                base::ASCIIToUTF16("Lisa"));
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                base::ASCIIToUTF16("grrrl@TV.com"));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, ConflictingFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                base::ASCIIToUTF16("Lisa"));
  UpdateProfile(1,
                GetAllAutoFillProfiles(1)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                base::ASCIIToUTF16("Bart"));

  // Don't care which write wins the conflict, only that the two clients agree.
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  base::string16 max_length_string(AutofillTable::kMaxDataLength, '.');
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FULL),
                max_length_string);
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                max_length_string);
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::ADDRESS_HOME_LINE1),
                max_length_string);

  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, ExceedsMaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  base::string16 exceeds_max_length_string(
      AutofillTable::kMaxDataLength + 1, '.');
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_FIRST),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::NAME_LAST),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::EMAIL_ADDRESS),
                exceeds_max_length_string);
  UpdateProfile(0,
                GetAllAutoFillProfiles(0)[0]->guid(),
                AutofillType(autofill::ADDRESS_HOME_LINE1),
                exceeds_max_length_string);

  ASSERT_TRUE(BookmarksMatchChecker().Wait());
  EXPECT_FALSE(ProfilesMatch(0, 1));
}

// Test credit cards don't sync.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, NoCreditCardSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  CreditCard card;
  card.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                  base::ASCIIToUTF16("6011111111111117"));
  std::vector<CreditCard> credit_cards{card};
  SetCreditCards(0, &credit_cards);

  AddProfile(0, CreateAutofillProfile(PROFILE_HOMER));

  // Because the credit card was created before the profile, if we wait for the
  // profile to sync between both clients, it should give the credit card enough
  // time to sync. We cannot directly wait/block for the credit card to sync
  // because we're expecting it to not sync.
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());
  ASSERT_EQ(1U, GetAllAutoFillProfiles(0).size());

  PersonalDataManager* pdm = GetPersonalDataManager(1);
  ASSERT_EQ(0U, pdm->GetCreditCards().size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
    E2E_ONLY(TwoClientsAddAutofillProfiles)) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // All profiles should sync same autofill profiles.
  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait())
      << "Initial autofill profiles did not match for all profiles.";

  // For clean profiles, the autofill profiles count should be zero. We are not
  // enforcing this, we only check that the final count is equal to initial
  // count plus new autofill profiles count.
  int init_autofill_profiles_count = GetProfileCount(0);

  // Add a new autofill profile to the first client.
  AddProfile(0, CreateUniqueAutofillProfile());

  ASSERT_TRUE(AutofillProfileChecker(0, 1).Wait());

  // Check that the total number of autofill profiles is as expected
  for (int i = 0; i < num_clients(); ++i) {
    ASSERT_EQ(GetProfileCount(i), init_autofill_profiles_count + 1) <<
        "Total autofill profile count is wrong.";
  }
}
