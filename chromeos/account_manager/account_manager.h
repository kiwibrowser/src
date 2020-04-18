// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ACCOUNT_MANAGER_ACCOUNT_MANAGER_H_
#define CHROMEOS_ACCOUNT_MANAGER_ACCOUNT_MANAGER_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "chromeos/account_manager/tokens.pb.h"
#include "chromeos/chromeos_export.h"

class OAuth2AccessTokenFetcher;
class OAuth2AccessTokenConsumer;

namespace base {
class SequencedTaskRunner;
class ImportantFileWriter;
}  // namespace base

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace chromeos {

class CHROMEOS_EXPORT AccountManager {
 public:
  struct AccountKey {
    // |id| is obfuscated GAIA id for |AccountType::ACCOUNT_TYPE_GAIA|.
    // |id| is object GUID (|AccountId::GetObjGuid|) for
    // |AccountType::ACCOUNT_TYPE_ACTIVE_DIRECTORY|.
    std::string id;
    account_manager::AccountType account_type;

    bool IsValid() const;

    bool operator<(const AccountKey& other) const;
    bool operator==(const AccountKey& other) const;
  };

  // A map from |AccountKey| to a raw token.
  using TokenMap = std::map<AccountKey, std::string>;

  // A callback for list of |AccountKey|s.
  using AccountListCallback = base::OnceCallback<void(std::vector<AccountKey>)>;

  class Observer {
   public:
    Observer();
    virtual ~Observer();

    // Called when the token for |account_key| is updated/inserted.
    // Use |AccountManager::AddObserver| to add an |Observer|.
    // Note: |Observer|s which register with |AccountManager| before its
    // initialization is complete will get notified when |AccountManager| is
    // fully initialized.
    // Note: |Observer|s which register with |AccountManager| after its
    // initialization is complete will not get an immediate
    // notification-on-registration.
    virtual void OnTokenUpserted(const AccountKey& account_key) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  // Note: |Initialize| MUST be called at least once on this object.
  AccountManager();
  ~AccountManager();

  // |home_dir| is the path of the Device Account's home directory (root of the
  // user's cryptohome). This method MUST be called at least once.
  void Initialize(const base::FilePath& home_dir);

  // Gets (async) a list of account keys known to |AccountManager|.
  void GetAccounts(AccountListCallback callback);

  // Updates or inserts a token, for the account corresponding to the given
  // |account_key|. |account_key| must be valid (|AccountKey::IsValid|).
  void UpsertToken(const AccountKey& account_key, const std::string& token);

  // Add a non owning pointer to an |AccountManager::Observer|.
  void AddObserver(Observer* observer);

  // Removes an |AccountManager::Observer|. Does nothing if the |observer| is
  // not in the list of known observers.
  void RemoveObserver(Observer* observer);

  // Creates and returns an |OAuth2AccessTokenFetcher| using the refresh token
  // stored for |account_key|. |IsTokenAvailable| should be |true| for
  // |account_key|, otherwise a |nullptr| is returned.
  std::unique_ptr<OAuth2AccessTokenFetcher> CreateAccessTokenFetcher(
      const AccountKey& account_key,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) const;

  // Returns |true| if an LST is available for |account_key|.
  // Note: An LST will not be available for |account_key| if it is an Active
  // Directory account.
  // Note: This method will return |false| if |AccountManager| has not been
  // initialized yet.
  bool IsTokenAvailable(const AccountKey& account_key) const;

 private:
  enum InitializationState {
    kNotStarted,   // Initialize has not been called
    kInProgress,   // Initialize has been called but not completed
    kInitialized,  // Initialization was successfully completed
  };

  friend class AccountManagerTest;
  FRIEND_TEST_ALL_PREFIXES(AccountManagerTest, TestInitialization);
  FRIEND_TEST_ALL_PREFIXES(AccountManagerTest, TestPersistence);

  // Initializes |AccountManager| with the provided |task_runner| and location
  // of the user's home directory.
  void Initialize(const base::FilePath& home_dir,
                  scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Reads tokens from |tokens| and inserts them in |tokens_| and runs all
  // callbacks waiting on |AccountManager| initialization.
  void InsertTokensAndRunInitializationCallbacks(const TokenMap& tokens);

  // Accepts a closure and runs it immediately if |AccountManager| has already
  // been initialized, otherwise saves the |closure| for running later, when the
  // class is initialized.
  void RunOnInitialization(base::OnceClosure closure);

  // Does the actual work of getting a list of accounts. Assumes that
  // |AccountManager| initialization (|init_state_|) is complete.
  void GetAccountsInternal(AccountListCallback callback);

  // Does the actual work of updating or inserting tokens. Assumes that
  // |AccountManager| initialization (|init_state_|) is complete.
  void UpsertTokenInternal(const AccountKey& account_key,
                           const std::string& token);

  // Posts a task on |task_runner_|, which is usually a background thread, to
  // persist the current state of |tokens_|.
  void PersistTokensAsync();

  // Notify |Observer|s about a token update.
  void NotifyTokenObservers(const AccountKey& account_key);

  // Status of this object's initialization.
  InitializationState init_state_ = InitializationState::kNotStarted;

  // A task runner for disk I/O.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<base::ImportantFileWriter> writer_;

  // A map of account keys to tokens.
  TokenMap tokens_;

  // Callbacks waiting on class initialization (|init_state_|).
  std::vector<base::OnceClosure> initialization_callbacks_;

  // A list of |AccountManager| observers.
  // Verifies that the list is empty on destruction.
  base::ObserverList<Observer, true /* check_empty */> observers_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AccountManager> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AccountManager);
};

// For logging.
CHROMEOS_EXPORT std::ostream& operator<<(
    std::ostream& os,
    const AccountManager::AccountKey& account_key);

}  // namespace chromeos

#endif  // CHROMEOS_ACCOUNT_MANAGER_ACCOUNT_MANAGER_H_
