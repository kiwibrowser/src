// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HASH_PASSWORD_MANAGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HASH_PASSWORD_MANAGER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"

class PrefService;

namespace password_manager {

// SyncPasswordData is being deprecated. Please use PasswordHashData instead.
struct SyncPasswordData {
  SyncPasswordData() = default;
  SyncPasswordData(const SyncPasswordData& other) = default;
  SyncPasswordData(const base::string16& password, bool force_update);
  bool MatchesPassword(const base::string16& password);

  size_t length;
  std::string salt;
  uint64_t hash;
  // Signal that we need to update password hash, salt, and length in profile
  // prefs.
  bool force_update;
};

struct PasswordHashData {
  PasswordHashData();
  PasswordHashData(const PasswordHashData& other);
  PasswordHashData(const std::string& username,
                   const base::string16& password,
                   bool force_update,
                   bool is_gaia_password = true);
  bool MatchesPassword(const std::string& username,
                       const base::string16& password,
                       bool is_gaia_password);

  std::string username;
  size_t length;
  std::string salt;
  uint64_t hash;
  bool force_update;
  bool is_gaia_password = true;
};

// Responsible for saving, clearing, retrieving and encryption of a password
// hash data in preferences.
// All methods should be called on UI thread.
class HashPasswordManager {
 public:
  HashPasswordManager() = default;
  explicit HashPasswordManager(PrefService* prefs);
  ~HashPasswordManager() = default;

  bool SavePasswordHash(const base::string16& password);
  bool SavePasswordHash(const std::string username,
                        const base::string16& password,
                        bool is_gaia_password = true);
  bool SavePasswordHash(const SyncPasswordData& sync_password_data);
  bool SavePasswordHash(const PasswordHashData& password_hash_data);
  void ClearSavedPasswordHash();
  void ClearSavedPasswordHash(const std::string& username,
                              bool is_gaia_password);

  // Returns empty if no hash is available.
  base::Optional<SyncPasswordData> RetrievePasswordHash();

  // Returns empty array if no hash is available.
  std::vector<PasswordHashData> RetrieveAllPasswordHashes();

  // Returns empty if no hash matching |username| and |is_gaia_password| is
  // available.
  base::Optional<PasswordHashData> RetrievePasswordHash(
      const std::string& username,
      bool is_gaia_password);

  // Whether |prefs_| has |kSyncPasswordHash| pref path.
  bool HasPasswordHash();

  // Whether password hash of |username| and |is_gaia_password| is stored.
  bool HasPasswordHash(const std::string& username, bool is_gaia_password);

  void set_prefs(PrefService* prefs) { prefs_ = prefs; }

  // During the deprecation of SyncPasswordData, migrates the sync password hash
  // that has already been captured.
  void MaybeMigrateExistingSyncPasswordHash(const std::string& sync_username);

  static std::string CreateRandomSalt();

  // Calculates 37 bits hash for a password. The calculation is based on a slow
  // hash function. The running time is ~10^{-4} seconds on Desktop.
  static uint64_t CalculatePasswordHash(const base::StringPiece16& text,
                                        const std::string& salt);

 private:
  // Saves encrypted string |s| in a preference |pref_name|. Returns true on
  // success.
  bool EncryptAndSaveToPrefs(const std::string& pref_name,
                             const std::string& s);

  // Encrypts and saves |password_hash_data| to prefs. Returns true on success.
  bool EncryptAndSave(const PasswordHashData& password_hash_data);

  // Retrieves and decrypts string value from a preference |pref_name|. Returns
  // an empty string on failure.
  std::string RetrievedDecryptedStringFromPrefs(const std::string& pref_name);

  PrefService* prefs_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(HashPasswordManager);
};

}  // namespace password_manager
#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HASH_PASSWORD_MANAGER_H_
