// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_CRYPTO_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_CRYPTO_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine/sync_encryption_handler.h"
#include "components/sync/engine/sync_engine.h"

namespace syncer {

class DataTypeManager;
class SyncPrefs;

// This class functions as mostly independent component of SyncServiceBase that
// handles things related to encryption, including holding lots of state and
// encryption communications with the sync thread.
class SyncServiceCrypto : public SyncEncryptionHandler::Observer {
 public:
  SyncServiceCrypto(const base::Closure& notify_observers,
                    const base::Callback<ModelTypeSet()>& get_preferred_types,
                    SyncPrefs* sync_prefs);
  ~SyncServiceCrypto() override;

  // See the SyncService header.
  base::Time GetExplicitPassphraseTime() const;
  bool IsUsingSecondaryPassphrase() const;
  void EnableEncryptEverything();
  bool IsEncryptEverythingEnabled() const;
  void SetEncryptionPassphrase(const std::string& passphrase, bool is_explicit);
  bool SetDecryptionPassphrase(const std::string& passphrase);

  // Returns the actual passphrase type being used for encryption.
  PassphraseType GetPassphraseType() const;

  // Returns true if encrypting all the sync data is allowed. If this method
  // returns false, EnableEncryptEverything() should not be called.
  bool IsEncryptEverythingAllowed() const;

  // Sets whether encrypting all the sync data is allowed or not.
  void SetEncryptEverythingAllowed(bool allowed);

  // Returns the current set of encrypted data types.
  ModelTypeSet GetEncryptedDataTypes() const;

  // SyncEncryptionHandler::Observer implementation.
  void OnPassphraseRequired(
      PassphraseRequiredReason reason,
      const sync_pb::EncryptedData& pending_keys) override;
  void OnPassphraseAccepted() override;
  void OnBootstrapTokenUpdated(const std::string& bootstrap_token,
                               BootstrapTokenType type) override;
  void OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                               bool encrypt_everything) override;
  void OnEncryptionComplete() override;
  void OnCryptographerStateChanged(Cryptographer* cryptographer) override;
  void OnPassphraseTypeChanged(PassphraseType type,
                               base::Time passphrase_time) override;
  void OnLocalSetPassphraseEncryption(
      const SyncEncryptionHandler::NigoriState& nigori_state) override;

  // Calls data type manager to start catch up configure.
  void BeginConfigureCatchUpBeforeClear();

  // Used to provide the engine and DTM when the engine is initialized.
  void SetSyncEngine(SyncEngine* engine) { engine_ = engine; }
  void SetDataTypeManager(DataTypeManager* dtm) { data_type_manager_ = dtm; }

  // Creates a proxy observer object that will post calls to this thread.
  std::unique_ptr<SyncEncryptionHandler::Observer> GetEncryptionObserverProxy();

  // Takes the previously saved nigori state; null if there isn't any.
  std::unique_ptr<SyncEncryptionHandler::NigoriState> TakeSavedNigoriState();

  PassphraseRequiredReason passphrase_required_reason() const {
    return passphrase_required_reason_;
  }
  bool encryption_pending() { return encryption_pending_; }

 private:
  // Checks if |passphrase| can be used to decrypt the cryptographer's pending
  // keys that were cached during NotifyPassphraseRequired. Returns true if
  // decryption was successful. Returns false otherwise. Must be called with a
  // non-empty pending keys cache.
  bool CheckPassphraseAgainstCachedPendingKeys(
      const std::string& passphrase) const;

  // Calls SyncServiceBase::NotifyObservers(). Never null.
  const base::Closure notify_observers_;

  // Calls SyncService::GetPreferredDataTypes(). Never null.
  const base::Callback<ModelTypeSet()> get_preferred_types_;

  // A pointer to the sync prefs. Never null and guaranteed to outlive us.
  SyncPrefs* const sync_prefs_;

  // These are only not-null when the engine is initialized.
  SyncEngine* engine_ = nullptr;
  DataTypeManager* data_type_manager_ = nullptr;

  // Was the last SYNC_PASSPHRASE_REQUIRED notification sent because it
  // was required for encryption, decryption with a cached passphrase, or
  // because a new passphrase is required?
  PassphraseRequiredReason passphrase_required_reason_ =
      REASON_PASSPHRASE_NOT_REQUIRED;

  // The current set of encrypted types.  Always a superset of
  // Cryptographer::SensitiveTypes().
  ModelTypeSet encrypted_types_ = SyncEncryptionHandler::SensitiveTypes();

  // Whether encrypting everything is allowed.
  bool encrypt_everything_allowed_ = true;

  // Whether we want to encrypt everything.
  bool encrypt_everything_ = false;

  // Whether we're waiting for an attempt to encryption all sync data to
  // complete. We track this at this layer in order to allow the user to cancel
  // if they e.g. don't remember their explicit passphrase.
  bool encryption_pending_ = false;

  // Nigori state after user switching to custom passphrase, saved until
  // transition steps complete. It will be injected into new engine after sync
  // restart.
  std::unique_ptr<SyncEncryptionHandler::NigoriState> saved_nigori_state_;

  // We cache the cryptographer's pending keys whenever NotifyPassphraseRequired
  // is called. This way, before the UI calls SetDecryptionPassphrase on the
  // syncer, it can avoid the overhead of an asynchronous decryption call and
  // give the user immediate feedback about the passphrase entered by first
  // trying to decrypt the cached pending keys on the UI thread. Note that
  // SetDecryptionPassphrase can still fail after the cached pending keys are
  // successfully decrypted if the pending keys have changed since the time they
  // were cached.
  sync_pb::EncryptedData cached_pending_keys_;

  // The state of the passphrase required to decrypt the bag of encryption keys
  // in the nigori node. Updated whenever a new nigori node arrives or the user
  // manually changes their passphrase state. Cached so we can synchronously
  // check it from the UI thread.
  PassphraseType cached_passphrase_type_ = PassphraseType::IMPLICIT_PASSPHRASE;

  // If an explicit passphrase is in use, the time at which the passphrase was
  // first set (if available).
  base::Time cached_explicit_passphrase_time_;

  base::ThreadChecker thread_checker_;
  base::WeakPtrFactory<SyncServiceCrypto> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncServiceCrypto);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_CRYPTO_H_
