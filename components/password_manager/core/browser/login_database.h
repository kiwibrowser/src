// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_DATABASE_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_DATABASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/pickle.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_change.h"
#include "components/password_manager/core/browser/psl_matching_helper.h"
#include "components/password_manager/core/browser/statistics_table.h"
#include "sql/connection.h"
#include "sql/meta_table.h"

#if defined(OS_IOS)
#include "base/gtest_prod_util.h"
#endif

namespace password_manager {

class SQLTableBuilder;

extern const int kCurrentVersionNumber;
extern const int kCompatibleVersionNumber;

// Interface to the database storage of login information, intended as a helper
// for PasswordStore on platforms that need internal storage of some or all of
// the login information.
class LoginDatabase {
 public:
  explicit LoginDatabase(const base::FilePath& db_path);
  virtual ~LoginDatabase();

  // Actually creates/opens the database. If false is returned, no other method
  // should be called.
  virtual bool Init();

  // Reports usage metrics to UMA.
  void ReportMetrics(const std::string& sync_username,
                     bool custom_passphrase_sync_enabled);

  // Adds |form| to the list of remembered password forms. Returns the list of
  // changes applied ({}, {ADD}, {REMOVE, ADD}). If it returns {REMOVE, ADD}
  // then the REMOVE is associated with the form that was added. Thus only the
  // primary key columns contain the values associated with the removed form.
  PasswordStoreChangeList AddLogin(const autofill::PasswordForm& form)
      WARN_UNUSED_RESULT;

  // Updates existing password form. Returns the list of applied changes
  // ({}, {UPDATE}). The password is looked up by the tuple {origin,
  // username_element, username_value, password_element, signon_realm}.
  // These columns stay intact.
  PasswordStoreChangeList UpdateLogin(const autofill::PasswordForm& form)
      WARN_UNUSED_RESULT;

  // Removes |form| from the list of remembered password forms. Returns true if
  // |form| was successfully removed from the database.
  bool RemoveLogin(const autofill::PasswordForm& form) WARN_UNUSED_RESULT;

  // Removes all logins created from |delete_begin| onwards (inclusive) and
  // before |delete_end|. You may use a null Time value to do an unbounded
  // delete in either direction.
  bool RemoveLoginsCreatedBetween(base::Time delete_begin,
                                  base::Time delete_end);

  // Removes all logins synced from |delete_begin| onwards (inclusive) and
  // before |delete_end|. You may use a null Time value to do an unbounded
  // delete in either direction.
  bool RemoveLoginsSyncedBetween(base::Time delete_begin,
                                 base::Time delete_end);

  // Sets the 'skip_zero_click' flag on all forms on |origin| to 'true'.
  bool DisableAutoSignInForOrigin(const GURL& origin);

  // All Get* methods below overwrite |forms| with the returned credentials. On
  // success, those methods return true.

  // Gets a list of credentials matching |form|, including blacklisted matches
  // and federated credentials.
  bool GetLogins(const PasswordStore::FormDigest& form,
                 std::vector<std::unique_ptr<autofill::PasswordForm>>* forms)
      const WARN_UNUSED_RESULT;

  // Retrieves all stored credentials with SCHEME_HTTP that have a realm whose
  // organization-identifying name -- that is, the first domain name label below
  // the effective TLD -- matches that of |signon_realm|. Return value indicates
  // a successful query (but potentially no results).
  //
  // For example, the organization-identifying name of "https://foo.example.org"
  // is `example`, and logins will be returned for "http://bar.example.co.uk",
  // but not for "http://notexample.com" or "https://example.foo.com".
  bool GetLoginsForSameOrganizationName(
      const std::string& signon_realm,
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) const;

  // Gets all logins created from |begin| onwards (inclusive) and before |end|.
  // You may use a null Time value to do an unbounded search in either
  // direction.
  bool GetLoginsCreatedBetween(
      base::Time begin,
      base::Time end,
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) const
      WARN_UNUSED_RESULT;

  // Gets all logins synced from |begin| onwards (inclusive) and before |end|.
  // You may use a null Time value to do an unbounded search in either
  // direction.
  bool GetLoginsSyncedBetween(
      base::Time begin,
      base::Time end,
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) const
      WARN_UNUSED_RESULT;

  // Gets the complete list of not blacklisted credentials.
  bool GetAutofillableLogins(
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) const
      WARN_UNUSED_RESULT;

  // Gets the complete list of blacklisted credentials.
  bool GetBlacklistLogins(std::vector<std::unique_ptr<autofill::PasswordForm>>*
                              forms) const WARN_UNUSED_RESULT;

  // Gets the list of auto-sign-inable credentials.
  bool GetAutoSignInLogins(std::vector<std::unique_ptr<autofill::PasswordForm>>*
                               forms) const WARN_UNUSED_RESULT;

  // Deletes the login database file on disk, and creates a new, empty database.
  // This can be used after migrating passwords to some other store, to ensure
  // that SQLite doesn't leave fragments of passwords in the database file.
  // Returns true on success; otherwise, whether the file was deleted and
  // whether further use of this login database will succeed is unspecified.
  bool DeleteAndRecreateDatabaseFile();

  // Returns the encrypted password value for the specified |form|.  Returns an
  // empty string if the row for this |form| is not found.
  std::string GetEncryptedPassword(const autofill::PasswordForm& form) const;

  StatisticsTable& stats_table() { return stats_table_; }

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // This instance should not encrypt/decrypt password values using OSCrypt.
  void disable_encryption() { use_encryption_ = false; }
#endif  // defined(OS_POSIX)

 private:
#if defined(OS_IOS)
  friend class LoginDatabaseIOSTest;
  FRIEND_TEST_ALL_PREFIXES(LoginDatabaseIOSTest, KeychainStorage);

  // On iOS, removes the keychain item that is used to store the
  // encrypted password for the supplied |form|.
  void DeleteEncryptedPassword(const autofill::PasswordForm& form);
#endif

  // Result values for encryption/decryption actions.
  enum EncryptionResult {
    // Success.
    ENCRYPTION_RESULT_SUCCESS,
    // Failure for a specific item (e.g., the encrypted value was manually
    // moved from another machine, and can't be decrypted on this machine).
    // This is presumed to be a permanent failure.
    ENCRYPTION_RESULT_ITEM_FAILURE,
    // A service-level failure (e.g., on a platform using a keyring, the keyring
    // is temporarily unavailable).
    // This is presumed to be a temporary failure.
    ENCRYPTION_RESULT_SERVICE_FAILURE,
  };

  // Encrypts plain_text, setting the value of cipher_text and returning true if
  // successful, or returning false and leaving cipher_text unchanged if
  // encryption fails (e.g., if the underlying OS encryption system is
  // temporarily unavailable).
  EncryptionResult EncryptedString(const base::string16& plain_text,
                                   std::string* cipher_text) const
      WARN_UNUSED_RESULT;

  // Decrypts cipher_text, setting the value of plain_text and returning true if
  // successful, or returning false and leaving plain_text unchanged if
  // decryption fails (e.g., if the underlying OS encryption system is
  // temporarily unavailable).
  EncryptionResult DecryptedString(const std::string& cipher_text,
                                   base::string16* plain_text) const
      WARN_UNUSED_RESULT;

  // Fills |form| from the values in the given statement (which is assumed to
  // be of the form used by the Get*Logins methods).
  // Returns the EncryptionResult from decrypting the password in |s|; if not
  // ENCRYPTION_RESULT_SUCCESS, |form| is not filled.
  EncryptionResult InitPasswordFormFromStatement(autofill::PasswordForm* form,
                                                 const sql::Statement& s) const
      WARN_UNUSED_RESULT;

  // Gets all blacklisted or all non-blacklisted (depending on |blacklisted|)
  // credentials. On success returns true and overwrites |forms| with the
  // result.
  bool GetAllLoginsWithBlacklistSetting(
      bool blacklisted,
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) const;

  // Overwrites |forms| with credentials retrieved from |statement|. If
  // |matched_form| is not null, filters out all results but those PSL-matching
  // |*matched_form| or federated credentials for it. On success returns true.
  bool StatementToForms(sql::Statement* statement,
                        const PasswordStore::FormDigest* matched_form,
                        std::vector<std::unique_ptr<autofill::PasswordForm>>*
                            forms) const WARN_UNUSED_RESULT;

  // Initializes all the *_statement_ data members with appropriate SQL
  // fragments based on |builder|.
  void InitializeStatementStrings(const SQLTableBuilder& builder);

  base::FilePath db_path_;
  mutable sql::Connection db_;
  sql::MetaTable meta_table_;
  StatisticsTable stats_table_;

  // These cached strings are used to build SQL statements.
  std::string add_statement_;
  std::string add_replace_statement_;
  std::string update_statement_;
  std::string delete_statement_;
  std::string autosignin_statement_;
  std::string get_statement_;
  std::string get_statement_psl_;
  std::string get_statement_federated_;
  std::string get_statement_psl_federated_;
  std::string get_same_organization_name_logins_statement_;
  std::string created_statement_;
  std::string synced_statement_;
  std::string blacklisted_statement_;
  std::string encrypted_statement_;

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // Whether password values should be encrypted.
  // TODO(crbug.com/571003) Only linux doesn't use encryption. Remove this once
  // Linux is fully migrated into LoginDatabase.
  bool use_encryption_ = true;
#endif  // defined(OS_POSIX)

  DISALLOW_COPY_AND_ASSIGN(LoginDatabase);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_DATABASE_H_
