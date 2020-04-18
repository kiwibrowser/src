// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_service_crypto.h"

#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/base/nigori.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/data_type_manager.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/sync_string_conversions.h"

namespace syncer {

namespace {

// A SyncEncryptionHandler::Observer implementation that simply posts all calls
// to another task runner.
class SyncEncryptionObserverProxy : public SyncEncryptionHandler::Observer {
 public:
  SyncEncryptionObserverProxy(
      base::WeakPtr<SyncEncryptionHandler::Observer> observer,
      scoped_refptr<base::SequencedTaskRunner> task_runner)
      : observer_(observer), task_runner_(std::move(task_runner)) {}

  void OnPassphraseRequired(
      PassphraseRequiredReason reason,
      const sync_pb::EncryptedData& pending_keys) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnPassphraseRequired,
                   observer_, reason, pending_keys));
  }

  void OnPassphraseAccepted() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnPassphraseAccepted,
                   observer_));
  }

  void OnBootstrapTokenUpdated(const std::string& bootstrap_token,
                               BootstrapTokenType type) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnBootstrapTokenUpdated,
                   observer_, bootstrap_token, type));
  }

  void OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                               bool encrypt_everything) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnEncryptedTypesChanged,
                   observer_, encrypted_types, encrypt_everything));
  }

  void OnEncryptionComplete() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnEncryptionComplete,
                   observer_));
  }

  void OnCryptographerStateChanged(Cryptographer* cryptographer) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &SyncEncryptionHandler::Observer::OnCryptographerStateChanged,
            observer_, cryptographer));
  }

  void OnPassphraseTypeChanged(PassphraseType type,
                               base::Time passphrase_time) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncEncryptionHandler::Observer::OnPassphraseTypeChanged,
                   observer_, type, passphrase_time));
  }

  void OnLocalSetPassphraseEncryption(
      const SyncEncryptionHandler::NigoriState& nigori_state) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &SyncEncryptionHandler::Observer::OnLocalSetPassphraseEncryption,
            observer_, nigori_state));
  }

 private:
  base::WeakPtr<SyncEncryptionHandler::Observer> observer_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace

SyncServiceCrypto::SyncServiceCrypto(
    const base::Closure& notify_observers,
    const base::Callback<ModelTypeSet()>& get_preferred_types,
    SyncPrefs* sync_prefs)
    : notify_observers_(notify_observers),
      get_preferred_types_(get_preferred_types),
      sync_prefs_(sync_prefs),
      weak_factory_(this) {
  DCHECK(notify_observers_);
  DCHECK(get_preferred_types_);
  DCHECK(sync_prefs_);
}

SyncServiceCrypto::~SyncServiceCrypto() = default;

base::Time SyncServiceCrypto::GetExplicitPassphraseTime() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return cached_explicit_passphrase_time_;
}

bool SyncServiceCrypto::IsUsingSecondaryPassphrase() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return cached_passphrase_type_ ==
             PassphraseType::FROZEN_IMPLICIT_PASSPHRASE ||
         cached_passphrase_type_ == PassphraseType::CUSTOM_PASSPHRASE;
}

void SyncServiceCrypto::EnableEncryptEverything() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsEncryptEverythingAllowed());
  DCHECK(engine_);

  // TODO(atwilson): Persist the encryption_pending_ flag to address the various
  // problems around cancelling encryption in the background (crbug.com/119649).
  if (!encrypt_everything_)
    encryption_pending_ = true;
}

bool SyncServiceCrypto::IsEncryptEverythingEnabled() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(engine_);
  return encrypt_everything_ || encryption_pending_;
}

void SyncServiceCrypto::SetEncryptionPassphrase(const std::string& passphrase,
                                                bool is_explicit) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // This should only be called when the engine has been initialized.
  DCHECK(engine_);
  DCHECK(data_type_manager_);
  DCHECK(!(!is_explicit && IsUsingSecondaryPassphrase()))
      << "Data is already encrypted using an explicit passphrase";
  DCHECK(!(is_explicit && passphrase_required_reason_ == REASON_DECRYPTION))
      << "Can not set explicit passphrase when decryption is needed.";

  DVLOG(1) << "Setting " << (is_explicit ? "explicit" : "implicit")
           << " passphrase for encryption.";
  if (passphrase_required_reason_ == REASON_ENCRYPTION) {
    // REASON_ENCRYPTION implies that the cryptographer does not have pending
    // keys. Hence, as long as we're not trying to do an invalid passphrase
    // change (e.g. explicit -> explicit or explicit -> implicit), we know this
    // will succeed. If for some reason a new encryption key arrives via
    // sync later, the SBH will trigger another OnPassphraseRequired().
    passphrase_required_reason_ = REASON_PASSPHRASE_NOT_REQUIRED;
    notify_observers_.Run();
  }

  if (!data_type_manager_->IsNigoriEnabled()) {
    NOTREACHED() << "SetEncryptionPassphrase must never be called when nigori"
                    " is disabled.";
    return;
  }

  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  // SetEncryptionPassphrase should never be called if we are currently
  // encrypted with an explicit passphrase.
  DCHECK(cached_passphrase_type_ == PassphraseType::KEYSTORE_PASSPHRASE ||
         cached_passphrase_type_ == PassphraseType::IMPLICIT_PASSPHRASE);

  engine_->SetEncryptionPassphrase(passphrase, is_explicit);
}

bool SyncServiceCrypto::SetDecryptionPassphrase(const std::string& passphrase) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(data_type_manager_);

  if (!data_type_manager_->IsNigoriEnabled()) {
    NOTREACHED() << "SetDecryptionPassphrase must never be called when nigori"
                    " is disabled.";
    return false;
  }

  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  // This should only be called when we have cached pending keys.
  DCHECK(cached_pending_keys_.has_blob());

  // Check the passphrase that was provided against our local cache of the
  // cryptographer's pending keys. If this was unsuccessful, the UI layer can
  // immediately call OnPassphraseRequired without showing the user a spinner.
  if (!CheckPassphraseAgainstCachedPendingKeys(passphrase))
    return false;

  engine_->SetDecryptionPassphrase(passphrase);

  // Since we were able to decrypt the cached pending keys with the passphrase
  // provided, we immediately alert the UI layer that the passphrase was
  // accepted. This will avoid the situation where a user enters a passphrase,
  // clicks OK, immediately reopens the advanced settings dialog, and gets an
  // unnecessary prompt for a passphrase.
  // Note: It is not guaranteed that the passphrase will be accepted by the
  // syncer thread, since we could receive a new nigori node while the task is
  // pending. This scenario is a valid race, and SetDecryptionPassphrase can
  // trigger a new OnPassphraseRequired if it needs to.
  OnPassphraseAccepted();
  return true;
}

PassphraseType SyncServiceCrypto::GetPassphraseType() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return cached_passphrase_type_;
}

bool SyncServiceCrypto::IsEncryptEverythingAllowed() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return encrypt_everything_allowed_;
}

void SyncServiceCrypto::SetEncryptEverythingAllowed(bool allowed) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(allowed || !engine_ || !IsEncryptEverythingEnabled());
  encrypt_everything_allowed_ = allowed;
}

ModelTypeSet SyncServiceCrypto::GetEncryptedDataTypes() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(encrypted_types_.Has(PASSWORDS));
  // We may be called during the setup process before we're
  // initialized.  In this case, we default to the sensitive types.
  return encrypted_types_;
}

void SyncServiceCrypto::OnPassphraseRequired(
    PassphraseRequiredReason reason,
    const sync_pb::EncryptedData& pending_keys) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Update our cache of the cryptographer's pending keys.
  cached_pending_keys_ = pending_keys;

  DVLOG(1) << "Passphrase required with reason: "
           << PassphraseRequiredReasonToString(reason);
  passphrase_required_reason_ = reason;

  const ModelTypeSet types = get_preferred_types_.Run();
  if (data_type_manager_) {
    DCHECK(data_type_manager_->IsNigoriEnabled());
    // Reconfigure without the encrypted types (excluded implicitly via the
    // failed datatypes handler).
    data_type_manager_->Configure(types, CONFIGURE_REASON_CRYPTO);
  }

  // Notify observers that the passphrase status may have changed.
  notify_observers_.Run();
}

void SyncServiceCrypto::OnPassphraseAccepted() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Clear our cache of the cryptographer's pending keys.
  cached_pending_keys_.clear_blob();

  // Reset passphrase_required_reason_ since we know we no longer require the
  // passphrase.
  passphrase_required_reason_ = REASON_PASSPHRASE_NOT_REQUIRED;

  // Make sure the data types that depend on the passphrase are started at
  // this time.
  const ModelTypeSet types = get_preferred_types_.Run();
  if (data_type_manager_) {
    // Re-enable any encrypted types if necessary.
    data_type_manager_->Configure(types, CONFIGURE_REASON_CRYPTO);
  }

  notify_observers_.Run();
}

void SyncServiceCrypto::OnBootstrapTokenUpdated(
    const std::string& bootstrap_token,
    BootstrapTokenType type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(sync_prefs_);
  if (type == PASSPHRASE_BOOTSTRAP_TOKEN) {
    sync_prefs_->SetEncryptionBootstrapToken(bootstrap_token);
  } else {
    sync_prefs_->SetKeystoreEncryptionBootstrapToken(bootstrap_token);
  }
}

void SyncServiceCrypto::OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                                                bool encrypt_everything) {
  DCHECK(thread_checker_.CalledOnValidThread());
  encrypted_types_ = encrypted_types;
  encrypt_everything_ = encrypt_everything;
  DCHECK(encrypt_everything_allowed_ || !encrypt_everything_);
  DVLOG(1) << "Encrypted types changed to "
           << ModelTypeSetToString(encrypted_types_)
           << " (encrypt everything is set to "
           << (encrypt_everything_ ? "true" : "false") << ")";
  DCHECK(encrypted_types_.Has(PASSWORDS));

  notify_observers_.Run();
}

void SyncServiceCrypto::OnEncryptionComplete() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Encryption complete";
  if (encryption_pending_ && encrypt_everything_) {
    encryption_pending_ = false;
    // This is to nudge the integration tests when encryption is
    // finished.
    notify_observers_.Run();
  }
}

void SyncServiceCrypto::OnCryptographerStateChanged(
    Cryptographer* cryptographer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Do nothing.
}

void SyncServiceCrypto::OnPassphraseTypeChanged(PassphraseType type,
                                                base::Time passphrase_time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Passphrase type changed to " << PassphraseTypeToString(type);
  cached_passphrase_type_ = type;
  cached_explicit_passphrase_time_ = passphrase_time;
}

void SyncServiceCrypto::OnLocalSetPassphraseEncryption(
    const SyncEncryptionHandler::NigoriState& nigori_state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!base::FeatureList::IsEnabled(
          switches::kSyncClearDataOnPassphraseEncryption))
    return;

  // At this point the user has set a custom passphrase and we have received the
  // updated nigori state. Time to cache the nigori state, and catch up the
  // active data types.
  UMA_HISTOGRAM_ENUMERATION("Sync.ClearServerDataEvents",
                            CLEAR_SERVER_DATA_STARTED, CLEAR_SERVER_DATA_MAX);
  sync_prefs_->SetNigoriSpecificsForPassphraseTransition(
      nigori_state.nigori_specifics);
  sync_prefs_->SetPassphraseEncryptionTransitionInProgress(true);
  BeginConfigureCatchUpBeforeClear();
}

void SyncServiceCrypto::BeginConfigureCatchUpBeforeClear() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(data_type_manager_);
  DCHECK(!saved_nigori_state_);
  saved_nigori_state_ = std::make_unique<SyncEncryptionHandler::NigoriState>();
  sync_prefs_->GetNigoriSpecificsForPassphraseTransition(
      &saved_nigori_state_->nigori_specifics);
  const ModelTypeSet types = data_type_manager_->GetActiveDataTypes();
  data_type_manager_->Configure(types, CONFIGURE_REASON_CATCH_UP);
}

std::unique_ptr<SyncEncryptionHandler::Observer>
SyncServiceCrypto::GetEncryptionObserverProxy() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::make_unique<SyncEncryptionObserverProxy>(
      weak_factory_.GetWeakPtr(), base::ThreadTaskRunnerHandle::Get());
}

std::unique_ptr<SyncEncryptionHandler::NigoriState>
SyncServiceCrypto::TakeSavedNigoriState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::move(saved_nigori_state_);
}

bool SyncServiceCrypto::CheckPassphraseAgainstCachedPendingKeys(
    const std::string& passphrase) const {
  DCHECK(cached_pending_keys_.has_blob());
  DCHECK(!passphrase.empty());
  Nigori nigori;
  nigori.InitByDerivation("localhost", "dummy", passphrase);
  std::string plaintext;
  bool result = nigori.Decrypt(cached_pending_keys_.blob(), &plaintext);
  DVLOG_IF(1, result) << "Passphrase failed to decrypt pending keys.";
  return result;
}

}  // namespace syncer
