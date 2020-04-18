// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/hash_password_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {
namespace {

class HashPasswordManagerTest : public testing::Test {
 public:
  HashPasswordManagerTest() {
    prefs_.registry()->RegisterStringPref(prefs::kSyncPasswordHash,
                                          std::string(),
                                          PrefRegistry::NO_REGISTRATION_FLAGS);
    prefs_.registry()->RegisterStringPref(prefs::kSyncPasswordLengthAndHashSalt,
                                          std::string(),
                                          PrefRegistry::NO_REGISTRATION_FLAGS);
    prefs_.registry()->RegisterListPref(prefs::kPasswordHashDataList,
                                        PrefRegistry::NO_REGISTRATION_FLAGS);
    // Mock OSCrypt. There is a call to OSCrypt on initializling
    // PasswordReuseDetector, so it should be mocked.
    OSCryptMocker::SetUp();
  }

  ~HashPasswordManagerTest() override { OSCryptMocker::TearDown(); }

 protected:
  TestingPrefServiceSimple prefs_;
};

TEST_F(HashPasswordManagerTest, SavingSyncPasswordData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  base::string16 password(base::UTF8ToUTF16("sync_password"));

  // Verify |SavePasswordHash(const base::string16&)| behavior.
  hash_password_manager.SavePasswordHash(password);
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));

  // Saves the same password again won't change password hash, length or salt.
  const std::string current_hash = prefs_.GetString(prefs::kSyncPasswordHash);
  const std::string current_length_and_salt =
      prefs_.GetString(prefs::kSyncPasswordLengthAndHashSalt);
  hash_password_manager.SavePasswordHash(password);
  EXPECT_EQ(current_hash, prefs_.GetString(prefs::kSyncPasswordHash));
  EXPECT_EQ(current_length_and_salt,
            prefs_.GetString(prefs::kSyncPasswordLengthAndHashSalt));

  // Verify |SavePasswordHash(const SyncPasswordData&)| behavior.
  base::string16 new_password(base::UTF8ToUTF16("new_sync_password"));
  SyncPasswordData sync_password_data(new_password, /*force_update=*/true);
  EXPECT_TRUE(sync_password_data.MatchesPassword(new_password));
  hash_password_manager.SavePasswordHash(sync_password_data);
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
}

TEST_F(HashPasswordManagerTest, SavingPasswordHashData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  base::string16 password(base::UTF8ToUTF16("password"));
  std::string username("user@example.com");

  // Verify |SavePasswordHash(const std::string,const base::string16&)|
  // behavior.
  hash_password_manager.SavePasswordHash(username, password,
                                         /*force_update=*/true);
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));

  // Saves the same password again won't change password hash, length or salt.
  base::Optional<PasswordHashData> current_password_hash_data =
      hash_password_manager.RetrievePasswordHash(username,
                                                 /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash(username, password,
                                         /*force_update=*/true);
  base::Optional<PasswordHashData> existing_password_data =
      hash_password_manager.RetrievePasswordHash(username,
                                                 /*is_gaia_password=*/true);
  EXPECT_EQ(current_password_hash_data->hash, existing_password_data->hash);
  EXPECT_EQ(current_password_hash_data->salt, existing_password_data->salt);
  EXPECT_TRUE(current_password_hash_data->is_gaia_password);
  EXPECT_TRUE(existing_password_data->is_gaia_password);

  // Verify |SavePasswordHash(const PasswordHashData&)| behavior.
  base::string16 new_password(base::UTF8ToUTF16("new_password"));
  PasswordHashData new_password_data(username, new_password,
                                     /*force_update=*/true);
  EXPECT_TRUE(hash_password_manager.SavePasswordHash(new_password_data));
  EXPECT_NE(current_password_hash_data->hash,
            hash_password_manager
                .RetrievePasswordHash(username, /*is_gaia_password=*/true)
                ->hash);
}

TEST_F(HashPasswordManagerTest, SavingGaiaPasswordAndNonGaiaPassword) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  base::string16 password(base::UTF8ToUTF16("password"));
  std::string username("user@example.com");

  // Saves a Gaia password.
  hash_password_manager.SavePasswordHash(username, password,
                                         /*is_gaia_password=*/true);
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  EXPECT_EQ(1u, hash_password_manager.RetrieveAllPasswordHashes().size());

  // Saves the same password again but this time it is not a Gaia password.
  hash_password_manager.SavePasswordHash(username, password,
                                         /*is_gaia_password=*/false);
  // Verifies that there should be two separate entry in the saved hash list.
  EXPECT_EQ(2u, hash_password_manager.RetrieveAllPasswordHashes().size());
}

TEST_F(HashPasswordManagerTest, SavingMultipleHashesAndRetrieveAll) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  base::string16 password(base::UTF8ToUTF16("password"));

  // Save password hash for 6 different users.
  hash_password_manager.SavePasswordHash("username1", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username2", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username3", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username4", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username5", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username6", password,
                                         /*is_gaia_password=*/true);
  hash_password_manager.SavePasswordHash("username3", password,
                                         /*is_gaia_password=*/false);

  // Since kMaxPasswordHashDataDictSize is set to 5, we will only save 5
  // password hashes that were most recently signed in.
  EXPECT_EQ(5u, hash_password_manager.RetrieveAllPasswordHashes().size());
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username1", /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username1", /*is_gaia_password=*/false));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username2", /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username2", /*is_gaia_password=*/false));
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username3",
                                                    /*is_gaia_password=*/true));
  EXPECT_TRUE(hash_password_manager.HasPasswordHash(
      "username3", /*is_gaia_password=*/false));
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username4",
                                                    /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username4", /*is_gaia_password=*/false));
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username5",
                                                    /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username5", /*is_gaia_password=*/false));
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username6",
                                                    /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username6", /*is_gaia_password=*/false));
}

TEST_F(HashPasswordManagerTest, ClearingSyncPasswordData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  hash_password_manager.SavePasswordHash(base::UTF8ToUTF16("sync_password"));
  hash_password_manager.ClearSavedPasswordHash();
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
}

TEST_F(HashPasswordManagerTest, ClearingPasswordHashData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  hash_password_manager.SavePasswordHash("username",
                                         base::UTF8ToUTF16("sync_password"),
                                         /*is_gaia_password=*/true);

  hash_password_manager.ClearSavedPasswordHash("other_username",
                                               /*is_gaia_password=*/true);
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username",
                                                    /*is_gaia_password=*/true));
  hash_password_manager.ClearSavedPasswordHash("username",
                                               /*is_gaia_password=*/false);
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("username",
                                                    /*is_gaia_password=*/true));

  hash_password_manager.ClearSavedPasswordHash("username",
                                               /*is_gaia_password=*/true);
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "username", /*is_gaia_password=*/true));
  EXPECT_EQ(0u, hash_password_manager.RetrieveAllPasswordHashes().size());
}

TEST_F(HashPasswordManagerTest, RetrievingSyncPasswordData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  hash_password_manager.SavePasswordHash(base::UTF8ToUTF16("sync_password"));
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kSyncPasswordLengthAndHashSalt));

  base::Optional<SyncPasswordData> sync_password_data =
      hash_password_manager.RetrievePasswordHash();
  ASSERT_TRUE(sync_password_data);
  EXPECT_EQ(13u, sync_password_data->length);
  EXPECT_EQ(16u, sync_password_data->salt.size());
  uint64_t expected_hash = HashPasswordManager::CalculatePasswordHash(
      base::UTF8ToUTF16("sync_password"), sync_password_data->salt);
  EXPECT_EQ(expected_hash, sync_password_data->hash);
}

TEST_F(HashPasswordManagerTest, RetrievingPasswordHashData) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kPasswordHashDataList));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  hash_password_manager.SavePasswordHash("username",
                                         base::UTF8ToUTF16("password"),
                                         /*is_gaia_password=*/true);
  EXPECT_EQ(1u, hash_password_manager.RetrieveAllPasswordHashes().size());

  base::Optional<PasswordHashData> password_hash_data =
      hash_password_manager.RetrievePasswordHash("username",
                                                 /*is_gaia_password=*/false);
  ASSERT_FALSE(password_hash_data);
  password_hash_data = hash_password_manager.RetrievePasswordHash(
      "username", /*is_gaia_password=*/true);
  ASSERT_TRUE(password_hash_data);
  EXPECT_EQ(8u, password_hash_data->length);
  EXPECT_EQ(16u, password_hash_data->salt.size());
  uint64_t expected_hash = HashPasswordManager::CalculatePasswordHash(
      base::UTF8ToUTF16("password"), password_hash_data->salt);
  EXPECT_EQ(expected_hash, password_hash_data->hash);

  base::Optional<PasswordHashData> non_existing_data =
      hash_password_manager.RetrievePasswordHash("non_existing_user", true);
  ASSERT_FALSE(non_existing_data);
}

TEST_F(HashPasswordManagerTest, CalculatePasswordHash) {
  const char* kPlainText[] = {"", "password", "password", "secret"};
  const char* kSalt[] = {"", "salt", "123", "456"};

  constexpr uint64_t kExpectedHash[] = {
      UINT64_C(0x1c610a7950), UINT64_C(0x1927dc525e), UINT64_C(0xf72f81aa6),
      UINT64_C(0x3645af77f),
  };

  static_assert(arraysize(kPlainText) == arraysize(kSalt),
                "Arrays must have the same size");
  static_assert(arraysize(kPlainText) == arraysize(kExpectedHash),
                "Arrays must have the same size");

  for (size_t i = 0; i < arraysize(kPlainText); ++i) {
    SCOPED_TRACE(i);
    base::string16 text = base::UTF8ToUTF16(kPlainText[i]);
    EXPECT_EQ(kExpectedHash[i],
              HashPasswordManager::CalculatePasswordHash(text, kSalt[i]));
  }
}

TEST_F(HashPasswordManagerTest, MigrateCapturedPasswordHash) {
  ASSERT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  HashPasswordManager hash_password_manager;
  hash_password_manager.set_prefs(&prefs_);
  hash_password_manager.SavePasswordHash(base::UTF8ToUTF16("sync_password"));
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  EXPECT_TRUE(prefs_.HasPrefPath(prefs::kSyncPasswordLengthAndHashSalt));

  hash_password_manager.MaybeMigrateExistingSyncPasswordHash("sync_username");
  EXPECT_TRUE(hash_password_manager.HasPasswordHash("sync_username",
                                                    /*is_gaia_password=*/true));
  EXPECT_FALSE(hash_password_manager.HasPasswordHash(
      "sync_username", /*is_gaia_password=*/false));
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordHash));
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kSyncPasswordLengthAndHashSalt));
}

}  // namespace
}  // namespace password_manager
