// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/sync_encryption_handler_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/json/json_string_value_serializer.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/base/encryptor.h"
#include "components/sync/base/experiments.h"
#include "components/sync/base/passphrase_type.h"
#include "components/sync/base/time.h"
#include "components/sync/engine/sync_string_conversions.h"
#include "components/sync/protocol/encryption.pb.h"
#include "components/sync/protocol/nigori_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/mutable_entry.h"
#include "components/sync/syncable/nigori_util.h"
#include "components/sync/syncable/read_node.h"
#include "components/sync/syncable/read_transaction.h"
#include "components/sync/syncable/syncable_base_transaction.h"
#include "components/sync/syncable/syncable_model_neutral_write_transaction.h"
#include "components/sync/syncable/syncable_write_transaction.h"
#include "components/sync/syncable/user_share.h"
#include "components/sync/syncable/write_node.h"
#include "components/sync/syncable/write_transaction.h"

namespace syncer {

namespace {

// The maximum number of times we will automatically overwrite the nigori node
// because the encryption keys don't match (per chrome instantiation).
// We protect ourselves against nigori rollbacks, but it's possible two
// different clients might have contrasting view of what the nigori node state
// should be, in which case they might ping pong (see crbug.com/119207).
static const int kNigoriOverwriteLimit = 10;

// Enumeration of nigori keystore migration results (for use in UMA stats).
enum NigoriMigrationResult {
  FAILED_TO_SET_DEFAULT_KEYSTORE,
  FAILED_TO_SET_NONDEFAULT_KEYSTORE,
  FAILED_TO_EXTRACT_DECRYPTOR,
  FAILED_TO_EXTRACT_KEYBAG,
  MIGRATION_SUCCESS_KEYSTORE_NONDEFAULT,
  MIGRATION_SUCCESS_KEYSTORE_DEFAULT,
  MIGRATION_SUCCESS_FROZEN_IMPLICIT,
  MIGRATION_SUCCESS_CUSTOM,
  MIGRATION_RESULT_SIZE,
};

enum NigoriMigrationState {
  MIGRATED,
  NOT_MIGRATED_CRYPTO_NOT_READY,
  NOT_MIGRATED_NO_KEYSTORE_KEY,
  NOT_MIGRATED_UNKNOWN_REASON,
  MIGRATION_STATE_SIZE,
};

// The new passphrase state is sufficient to determine whether a nigori node
// is migrated to support keystore encryption. In addition though, we also
// want to verify the conditions for proper keystore encryption functionality.
// 1. Passphrase type is set.
// 2. Frozen keybag is true
// 3. If passphrase state is keystore, keystore_decryptor_token is set.
bool IsNigoriMigratedToKeystore(const sync_pb::NigoriSpecifics& nigori) {
  if (!nigori.has_passphrase_type())
    return false;
  if (!nigori.keybag_is_frozen())
    return false;
  if (nigori.passphrase_type() == sync_pb::NigoriSpecifics::IMPLICIT_PASSPHRASE)
    return false;
  if (nigori.passphrase_type() ==
          sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE &&
      nigori.keystore_decryptor_token().blob().empty())
    return false;
  return true;
}

PassphraseType ProtoPassphraseTypeToEnum(
    sync_pb::NigoriSpecifics::PassphraseType type) {
  switch (type) {
    case sync_pb::NigoriSpecifics::IMPLICIT_PASSPHRASE:
      return PassphraseType::IMPLICIT_PASSPHRASE;
    case sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE:
      return PassphraseType::KEYSTORE_PASSPHRASE;
    case sync_pb::NigoriSpecifics::CUSTOM_PASSPHRASE:
      return PassphraseType::CUSTOM_PASSPHRASE;
    case sync_pb::NigoriSpecifics::FROZEN_IMPLICIT_PASSPHRASE:
      return PassphraseType::FROZEN_IMPLICIT_PASSPHRASE;
    default:
      NOTREACHED();
      return PassphraseType::IMPLICIT_PASSPHRASE;
  }
}

sync_pb::NigoriSpecifics::PassphraseType EnumPassphraseTypeToProto(
    PassphraseType type) {
  switch (type) {
    case PassphraseType::IMPLICIT_PASSPHRASE:
      return sync_pb::NigoriSpecifics::IMPLICIT_PASSPHRASE;
    case PassphraseType::KEYSTORE_PASSPHRASE:
      return sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE;
    case PassphraseType::CUSTOM_PASSPHRASE:
      return sync_pb::NigoriSpecifics::CUSTOM_PASSPHRASE;
    case PassphraseType::FROZEN_IMPLICIT_PASSPHRASE:
      return sync_pb::NigoriSpecifics::FROZEN_IMPLICIT_PASSPHRASE;
    default:
      NOTREACHED();
      return sync_pb::NigoriSpecifics::IMPLICIT_PASSPHRASE;
  }
}

// Keystore Bootstrap Token helper methods.
// The bootstrap is a base64 encoded, encrypted, ListValue of keystore key
// strings, with the current keystore key as the last value in the list.
std::string PackKeystoreBootstrapToken(
    const std::vector<std::string>& old_keystore_keys,
    const std::string& current_keystore_key,
    Encryptor* encryptor) {
  if (current_keystore_key.empty())
    return std::string();

  base::ListValue keystore_key_values;
  for (size_t i = 0; i < old_keystore_keys.size(); ++i)
    keystore_key_values.AppendString(old_keystore_keys[i]);
  keystore_key_values.AppendString(current_keystore_key);

  // Update the bootstrap token.
  // The bootstrap is a base64 encoded, encrypted, ListValue of keystore key
  // strings, with the current keystore key as the last value in the list.
  std::string serialized_keystores;
  JSONStringValueSerializer json(&serialized_keystores);
  json.Serialize(keystore_key_values);
  std::string encrypted_keystores;
  encryptor->EncryptString(serialized_keystores, &encrypted_keystores);
  std::string keystore_bootstrap;
  base::Base64Encode(encrypted_keystores, &keystore_bootstrap);
  return keystore_bootstrap;
}

bool UnpackKeystoreBootstrapToken(const std::string& keystore_bootstrap_token,
                                  Encryptor* encryptor,
                                  std::vector<std::string>* old_keystore_keys,
                                  std::string* current_keystore_key) {
  if (keystore_bootstrap_token.empty())
    return false;
  std::string base64_decoded_keystore_bootstrap;
  if (!base::Base64Decode(keystore_bootstrap_token,
                          &base64_decoded_keystore_bootstrap)) {
    return false;
  }
  std::string decrypted_keystore_bootstrap;
  if (!encryptor->DecryptString(base64_decoded_keystore_bootstrap,
                                &decrypted_keystore_bootstrap)) {
    return false;
  }

  JSONStringValueDeserializer json(decrypted_keystore_bootstrap);
  std::unique_ptr<base::Value> deserialized_keystore_keys(
      json.Deserialize(nullptr, nullptr));
  if (!deserialized_keystore_keys)
    return false;
  base::ListValue* internal_list_value = nullptr;
  if (!deserialized_keystore_keys->GetAsList(&internal_list_value))
    return false;
  int number_of_keystore_keys = internal_list_value->GetSize();
  if (!internal_list_value->GetString(number_of_keystore_keys - 1,
                                      current_keystore_key)) {
    return false;
  }
  old_keystore_keys->resize(number_of_keystore_keys - 1);
  for (int i = 0; i < number_of_keystore_keys - 1; ++i)
    internal_list_value->GetString(i, &(*old_keystore_keys)[i]);
  return true;
}

}  // namespace

SyncEncryptionHandlerImpl::Vault::Vault(Encryptor* encryptor,
                                        ModelTypeSet encrypted_types,
                                        PassphraseType passphrase_type)
    : cryptographer(encryptor),
      encrypted_types(encrypted_types),
      passphrase_type(passphrase_type) {}

SyncEncryptionHandlerImpl::Vault::~Vault() {}

SyncEncryptionHandlerImpl::SyncEncryptionHandlerImpl(
    UserShare* user_share,
    Encryptor* encryptor,
    const std::string& restored_key_for_bootstrapping,
    const std::string& restored_keystore_key_for_bootstrapping)
    : user_share_(user_share),
      vault_unsafe_(encryptor,
                    SensitiveTypes(),
                    PassphraseType::IMPLICIT_PASSPHRASE),
      encrypt_everything_(false),
      nigori_overwrite_count_(0),
      weak_ptr_factory_(this) {
  // Restore the cryptographer's previous keys. Note that we don't add the
  // keystore keys into the cryptographer here, in case a migration was pending.
  vault_unsafe_.cryptographer.Bootstrap(restored_key_for_bootstrapping);

  // If this fails, we won't have a valid keystore key, and will simply request
  // new ones from the server on the next DownloadUpdates.
  UnpackKeystoreBootstrapToken(restored_keystore_key_for_bootstrapping,
                               encryptor, &old_keystore_keys_, &keystore_key_);
}

SyncEncryptionHandlerImpl::~SyncEncryptionHandlerImpl() {}

void SyncEncryptionHandlerImpl::AddObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!observers_.HasObserver(observer));
  observers_.AddObserver(observer);
}

void SyncEncryptionHandlerImpl::RemoveObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(observers_.HasObserver(observer));
  observers_.RemoveObserver(observer);
}

void SyncEncryptionHandlerImpl::Init() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  WriteNode node(&trans);

  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK)
    return;
  if (!ApplyNigoriUpdateImpl(node.GetNigoriSpecifics(),
                             trans.GetWrappedTrans())) {
    WriteEncryptionStateToNigori(&trans);
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Sync.PassphraseType",
      static_cast<unsigned>(GetPassphraseType(trans.GetWrappedTrans())),
      static_cast<unsigned>(PassphraseType::PASSPHRASE_TYPE_SIZE));

  bool has_pending_keys =
      UnlockVault(trans.GetWrappedTrans()).cryptographer.has_pending_keys();
  bool is_ready = UnlockVault(trans.GetWrappedTrans()).cryptographer.is_ready();
  // Log the state of the cryptographer regardless of migration state.
  UMA_HISTOGRAM_BOOLEAN("Sync.CryptographerReady", is_ready);
  UMA_HISTOGRAM_BOOLEAN("Sync.CryptographerPendingKeys", has_pending_keys);
  if (IsNigoriMigratedToKeystore(node.GetNigoriSpecifics())) {
    // This account has a nigori node that has been migrated to support
    // keystore.
    UMA_HISTOGRAM_ENUMERATION("Sync.NigoriMigrationState", MIGRATED,
                              MIGRATION_STATE_SIZE);
    if (has_pending_keys &&
        GetPassphraseType(trans.GetWrappedTrans()) ==
            PassphraseType::KEYSTORE_PASSPHRASE) {
      // If this is happening, it means the keystore decryptor is either
      // undecryptable with the available keystore keys or does not match the
      // nigori keybag's encryption key. Otherwise we're simply missing the
      // keystore key.
      UMA_HISTOGRAM_BOOLEAN("Sync.KeystoreDecryptionFailed",
                            !keystore_key_.empty());
    }
  } else if (!is_ready) {
    // Migration cannot occur until the cryptographer is ready (initialized
    // with GAIA password and any pending keys resolved).
    UMA_HISTOGRAM_ENUMERATION("Sync.NigoriMigrationState",
                              NOT_MIGRATED_CRYPTO_NOT_READY,
                              MIGRATION_STATE_SIZE);
  } else if (keystore_key_.empty()) {
    // The client has no keystore key, either because it is not yet enabled or
    // the server is not sending a valid keystore key.
    UMA_HISTOGRAM_ENUMERATION("Sync.NigoriMigrationState",
                              NOT_MIGRATED_NO_KEYSTORE_KEY,
                              MIGRATION_STATE_SIZE);
  } else {
    // If the above conditions have been met and the nigori node is still not
    // migrated, something failed in the migration process.
    UMA_HISTOGRAM_ENUMERATION("Sync.NigoriMigrationState",
                              NOT_MIGRATED_UNKNOWN_REASON,
                              MIGRATION_STATE_SIZE);
  }

  // Always trigger an encrypted types and cryptographer state change event at
  // init time so observers get the initial values.
  for (auto& observer : observers_) {
    observer.OnEncryptedTypesChanged(
        UnlockVault(trans.GetWrappedTrans()).encrypted_types,
        encrypt_everything_);
  }
  for (auto& observer : observers_) {
    observer.OnCryptographerStateChanged(
        &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer);
  }

  // If the cryptographer is not ready (either it has pending keys or we
  // failed to initialize it), we don't want to try and re-encrypt the data.
  // If we had encrypted types, the DataTypeManager will block, preventing
  // sync from happening until the the passphrase is provided.
  if (UnlockVault(trans.GetWrappedTrans()).cryptographer.is_ready())
    ReEncryptEverything(&trans);
}

void SyncEncryptionHandlerImpl::SetEncryptionPassphrase(
    const std::string& passphrase,
    bool is_explicit) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We do not accept empty passphrases.
  if (passphrase.empty()) {
    NOTREACHED() << "Cannot encrypt with an empty passphrase.";
    return;
  }

  // All accesses to the cryptographer are protected by a transaction.
  WriteTransaction trans(FROM_HERE, user_share_);
  KeyParams key_params = {"localhost", "dummy", passphrase};
  WriteNode node(&trans);
  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK) {
    NOTREACHED();
    return;
  }

  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer;

  // Once we've migrated to keystore, the only way to set a passphrase for
  // encryption is to set a custom passphrase.
  if (IsNigoriMigratedToKeystore(node.GetNigoriSpecifics())) {
    if (!is_explicit) {
      // The user is setting a new implicit passphrase. At this point we don't
      // care, so drop it on the floor. This is safe because if we have a
      // migrated nigori node, then we don't need to create an initial
      // encryption key.
      LOG(WARNING) << "Ignoring new implicit passphrase. Keystore migration "
                   << "already performed.";
      return;
    }
    // Will fail if we already have an explicit passphrase or we have pending
    // keys.
    SetCustomPassphrase(passphrase, &trans, &node);

    // When keystore migration occurs, the "CustomEncryption" UMA stat must be
    // logged as true.
    UMA_HISTOGRAM_BOOLEAN("Sync.CustomEncryption", true);
    return;
  }

  std::string bootstrap_token;
  sync_pb::EncryptedData pending_keys;
  if (cryptographer->has_pending_keys())
    pending_keys = cryptographer->GetPendingKeys();
  bool success = false;
  PassphraseType* passphrase_type =
      &UnlockVaultMutable(trans.GetWrappedTrans())->passphrase_type;
  // There are six cases to handle here:
  // 1. The user has no pending keys and is setting their current GAIA password
  //    as the encryption passphrase. This happens either during first time sync
  //    with a clean profile, or after re-authenticating on a profile that was
  //    already signed in with the cryptographer ready.
  // 2. The user has no pending keys, and is overwriting an (already provided)
  //    implicit passphrase with an explicit (custom) passphrase.
  // 3. The user has pending keys for an explicit passphrase that is somehow set
  //    to their current GAIA passphrase.
  // 4. The user has pending keys encrypted with their current GAIA passphrase
  //    and the caller passes in the current GAIA passphrase.
  // 5. The user has pending keys encrypted with an older GAIA passphrase
  //    and the caller passes in the current GAIA passphrase.
  // 6. The user has previously done encryption with an explicit passphrase.
  // Furthermore, we enforce the fact that the bootstrap encryption token will
  // always be derived from the newest GAIA password if the account is using
  // an implicit passphrase (even if the data is encrypted with an old GAIA
  // password). If the account is using an explicit (custom) passphrase, the
  // bootstrap token will be derived from the most recently provided explicit
  // passphrase (that was able to decrypt the data).
  if (!IsExplicitPassphrase(*passphrase_type)) {
    if (!cryptographer->has_pending_keys()) {
      if (cryptographer->AddKey(key_params)) {
        // Case 1 and 2. We set a new GAIA passphrase when there are no pending
        // keys (1), or overwriting an implicit passphrase with a new explicit
        // one (2) when there are no pending keys.
        if (is_explicit) {
          DVLOG(1) << "Setting explicit passphrase for encryption.";
          *passphrase_type = PassphraseType::CUSTOM_PASSPHRASE;
          custom_passphrase_time_ = base::Time::Now();
          for (auto& observer : observers_) {
            observer.OnPassphraseTypeChanged(
                *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
          }
        } else {
          DVLOG(1) << "Setting implicit passphrase for encryption.";
        }
        cryptographer->GetBootstrapToken(&bootstrap_token);

        // With M26, sync accounts can be in only one of two encryption states:
        // 1) Encrypt only passwords with an implicit passphrase.
        // 2) Encrypt all sync datatypes with an explicit passphrase.
        // We deprecate the "EncryptAllData" and "CustomPassphrase" histograms,
        // and keep track of an account's encryption state via the
        // "CustomEncryption" histogram. See http://crbug.com/131478.
        UMA_HISTOGRAM_BOOLEAN("Sync.CustomEncryption", is_explicit);

        success = true;
      } else {
        NOTREACHED() << "Failed to add key to cryptographer.";
        success = false;
      }
    } else {  // cryptographer->has_pending_keys() == true
      if (is_explicit) {
        // This can only happen if the nigori node is updated with a new
        // implicit passphrase while a client is attempting to set a new custom
        // passphrase (race condition).
        DVLOG(1) << "Failing because an implicit passphrase is already set.";
        success = false;
      } else {  // is_explicit == false
        if (cryptographer->DecryptPendingKeys(key_params)) {
          // Case 4. We successfully decrypted with the implicit GAIA passphrase
          // passed in.
          DVLOG(1) << "Implicit internal passphrase accepted for decryption.";
          cryptographer->GetBootstrapToken(&bootstrap_token);
          success = true;
        } else {
          // Case 5. Encryption was done with an old GAIA password, but we were
          // provided with the current GAIA password. We need to generate a new
          // bootstrap token to preserve it. We build a temporary cryptographer
          // to allow us to extract these params without polluting our current
          // cryptographer.
          DVLOG(1) << "Implicit internal passphrase failed to decrypt, adding "
                   << "anyways as default passphrase and persisting via "
                   << "bootstrap token.";
          Cryptographer temp_cryptographer(cryptographer->encryptor());
          temp_cryptographer.AddKey(key_params);
          temp_cryptographer.GetBootstrapToken(&bootstrap_token);
          // We then set the new passphrase as the default passphrase of the
          // real cryptographer, even though we have pending keys. This is safe,
          // as although Cryptographer::is_initialized() will now be true,
          // is_ready() will remain false due to having pending keys.
          cryptographer->AddKey(key_params);
          success = false;
        }
      }     // is_explicit
    }       // cryptographer->has_pending_keys()
  } else {  // IsExplicitPassphrase(passphrase_type) == true.
    // Case 6. We do not want to override a previously set explicit passphrase,
    // so we return a failure.
    DVLOG(1) << "Failing because an explicit passphrase is already set.";
    success = false;
  }

  DVLOG_IF(1, !success)
      << "Failure in SetEncryptionPassphrase; notifying and returning.";
  DVLOG_IF(1, success)
      << "Successfully set encryption passphrase; updating nigori and "
         "reencrypting.";

  FinishSetPassphrase(success, bootstrap_token, &trans, &node);
}

void SyncEncryptionHandlerImpl::SetDecryptionPassphrase(
    const std::string& passphrase) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We do not accept empty passphrases.
  if (passphrase.empty()) {
    NOTREACHED() << "Cannot decrypt with an empty passphrase.";
    return;
  }

  // All accesses to the cryptographer are protected by a transaction.
  WriteTransaction trans(FROM_HERE, user_share_);
  KeyParams key_params = {"localhost", "dummy", passphrase};
  WriteNode node(&trans);
  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK) {
    NOTREACHED();
    return;
  }

  // Once we've migrated to keystore, we're only ever decrypting keys derived
  // from an explicit passphrase. But, for clients without a keystore key yet
  // (either not on by default or failed to download one), we still support
  // decrypting with a gaia passphrase, and therefore bypass the
  // DecryptPendingKeysWithExplicitPassphrase logic.
  if (IsNigoriMigratedToKeystore(node.GetNigoriSpecifics()) &&
      IsExplicitPassphrase(GetPassphraseType(trans.GetWrappedTrans()))) {
    DecryptPendingKeysWithExplicitPassphrase(passphrase, &trans, &node);
    return;
  }

  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer;
  if (!cryptographer->has_pending_keys()) {
    // Note that this *can* happen in a rare situation where data is
    // re-encrypted on another client while a SetDecryptionPassphrase() call is
    // in-flight on this client. It is rare enough that we choose to do nothing.
    NOTREACHED() << "Attempt to set decryption passphrase failed because there "
                 << "were no pending keys.";
    return;
  }

  std::string bootstrap_token;
  sync_pb::EncryptedData pending_keys;
  pending_keys = cryptographer->GetPendingKeys();
  bool success = false;

  // There are three cases to handle here:
  // 7. We're using the current GAIA password to decrypt the pending keys. This
  //    happens when signing in to an account with a previously set implicit
  //    passphrase, where the data is already encrypted with the newest GAIA
  //    password.
  // 8. The user is providing an old GAIA password to decrypt the pending keys.
  //    In this case, the user is using an implicit passphrase, but has changed
  //    their password since they last encrypted their data, and therefore
  //    their current GAIA password was unable to decrypt the data. This will
  //    happen when the user is setting up a new profile with a previously
  //    encrypted account (after changing passwords).
  // 9. The user is providing a previously set explicit passphrase to decrypt
  //    the pending keys.
  if (!IsExplicitPassphrase(GetPassphraseType(trans.GetWrappedTrans()))) {
    if (cryptographer->is_initialized()) {
      // We only want to change the default encryption key to the pending
      // one if the pending keybag already contains the current default.
      // This covers the case where a different client re-encrypted
      // everything with a newer gaia passphrase (and hence the keybag
      // contains keys from all previously used gaia passphrases).
      // Otherwise, we're in a situation where the pending keys are
      // encrypted with an old gaia passphrase, while the default is the
      // current gaia passphrase. In that case, we preserve the default.
      Cryptographer temp_cryptographer(cryptographer->encryptor());
      temp_cryptographer.SetPendingKeys(cryptographer->GetPendingKeys());
      if (temp_cryptographer.DecryptPendingKeys(key_params)) {
        // Check to see if the pending bag of keys contains the current
        // default key.
        sync_pb::EncryptedData encrypted;
        cryptographer->GetKeys(&encrypted);
        if (temp_cryptographer.CanDecrypt(encrypted)) {
          DVLOG(1) << "Implicit user provided passphrase accepted for "
                   << "decryption, overwriting default.";
          // Case 7. The pending keybag contains the current default. Go ahead
          // and update the cryptographer, letting the default change.
          cryptographer->DecryptPendingKeys(key_params);
          cryptographer->GetBootstrapToken(&bootstrap_token);
          success = true;
        } else {
          // Case 8. The pending keybag does not contain the current default
          // encryption key. We decrypt the pending keys here, and in
          // FinishSetPassphrase, re-encrypt everything with the current GAIA
          // passphrase instead of the passphrase just provided by the user.
          DVLOG(1) << "Implicit user provided passphrase accepted for "
                   << "decryption, restoring implicit internal passphrase "
                   << "as default.";
          std::string bootstrap_token_from_current_key;
          cryptographer->GetBootstrapToken(&bootstrap_token_from_current_key);
          cryptographer->DecryptPendingKeys(key_params);
          // Overwrite the default from the pending keys.
          cryptographer->AddKeyFromBootstrapToken(
              bootstrap_token_from_current_key);
          success = true;
        }
      } else {  // !temp_cryptographer.DecryptPendingKeys(..)
        DVLOG(1) << "Implicit user provided passphrase failed to decrypt.";
        success = false;
      }       // temp_cryptographer.DecryptPendingKeys(...)
    } else {  // cryptographer->is_initialized() == false
      if (cryptographer->DecryptPendingKeys(key_params)) {
        // This can happpen in two cases:
        // - First time sync on android, where we'll never have a
        //   !user_provided passphrase.
        // - This is a restart for a client that lost their bootstrap token.
        // In both cases, we should go ahead and initialize the cryptographer
        // and persist the new bootstrap token.
        //
        // Note: at this point, we cannot distinguish between cases 7 and 8
        // above. This user provided passphrase could be the current or the
        // old. But, as long as we persist the token, there's nothing more
        // we can do.
        cryptographer->GetBootstrapToken(&bootstrap_token);
        DVLOG(1) << "Implicit user provided passphrase accepted, initializing"
                 << " cryptographer.";
        success = true;
      } else {
        DVLOG(1) << "Implicit user provided passphrase failed to decrypt.";
        success = false;
      }
    }       // cryptographer->is_initialized()
  } else {  // nigori_has_explicit_passphrase == true
    // Case 9. Encryption was done with an explicit passphrase, and we decrypt
    // with the passphrase provided by the user.
    if (cryptographer->DecryptPendingKeys(key_params)) {
      DVLOG(1) << "Explicit passphrase accepted for decryption.";
      cryptographer->GetBootstrapToken(&bootstrap_token);
      success = true;
    } else {
      DVLOG(1) << "Explicit passphrase failed to decrypt.";
      success = false;
    }
  }  // nigori_has_explicit_passphrase

  DVLOG_IF(1, !success)
      << "Failure in SetDecryptionPassphrase; notifying and returning.";
  DVLOG_IF(1, success)
      << "Successfully set decryption passphrase; updating nigori and "
         "reencrypting.";

  FinishSetPassphrase(success, bootstrap_token, &trans, &node);
}

void SyncEncryptionHandlerImpl::EnableEncryptEverything() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  DVLOG(1) << "Enabling encrypt everything.";
  if (encrypt_everything_)
    return;
  EnableEncryptEverythingImpl(trans.GetWrappedTrans());
  WriteEncryptionStateToNigori(&trans);
  if (UnlockVault(trans.GetWrappedTrans()).cryptographer.is_ready())
    ReEncryptEverything(&trans);
}

bool SyncEncryptionHandlerImpl::IsEncryptEverythingEnabled() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return encrypt_everything_;
}

// Note: this is called from within a syncable transaction, so we need to post
// tasks if we want to do any work that creates a new sync_api transaction.
void SyncEncryptionHandlerImpl::ApplyNigoriUpdate(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(trans);
  if (!ApplyNigoriUpdateImpl(nigori, trans)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&SyncEncryptionHandlerImpl::RewriteNigori,
                              weak_ptr_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnCryptographerStateChanged(
        &UnlockVaultMutable(trans)->cryptographer);
  }
}

void SyncEncryptionHandlerImpl::UpdateNigoriFromEncryptedTypes(
    sync_pb::NigoriSpecifics* nigori,
    syncable::BaseTransaction* const trans) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  syncable::UpdateNigoriFromEncryptedTypes(UnlockVault(trans).encrypted_types,
                                           encrypt_everything_, nigori);
}

bool SyncEncryptionHandlerImpl::NeedKeystoreKey(
    syncable::BaseTransaction* const trans) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return keystore_key_.empty();
}

bool SyncEncryptionHandlerImpl::SetKeystoreKeys(
    const google::protobuf::RepeatedPtrField<google::protobuf::string>& keys,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (keys.size() == 0)
    return false;
  // The last key in the vector is the current keystore key. The others are kept
  // around for decryption only.
  const std::string& raw_keystore_key = keys.Get(keys.size() - 1);
  if (raw_keystore_key.empty())
    return false;

  // Note: in order to Pack the keys, they must all be base64 encoded (else
  // JSON serialization fails).
  base::Base64Encode(raw_keystore_key, &keystore_key_);

  // Go through and save the old keystore keys. We always persist all keystore
  // keys the server sends us.
  old_keystore_keys_.resize(keys.size() - 1);
  for (int i = 0; i < keys.size() - 1; ++i)
    base::Base64Encode(keys.Get(i), &old_keystore_keys_[i]);

  Cryptographer* cryptographer = &UnlockVaultMutable(trans)->cryptographer;

  // Update the bootstrap token. If this fails, we persist an empty string,
  // which will force us to download the keystore keys again on the next
  // restart.
  std::string keystore_bootstrap = PackKeystoreBootstrapToken(
      old_keystore_keys_, keystore_key_, cryptographer->encryptor());

  for (auto& observer : observers_) {
    observer.OnBootstrapTokenUpdated(keystore_bootstrap,
                                     KEYSTORE_BOOTSTRAP_TOKEN);
  }
  DVLOG(1) << "Keystore bootstrap token updated.";

  // If this is a first time sync, we get the encryption keys before we process
  // the nigori node. Just return for now, ApplyNigoriUpdate will be invoked
  // once we have the nigori node.
  syncable::Entry entry(trans, syncable::GET_TYPE_ROOT, NIGORI);
  if (!entry.good())
    return true;

  const sync_pb::NigoriSpecifics& nigori = entry.GetSpecifics().nigori();
  if (cryptographer->has_pending_keys() && IsNigoriMigratedToKeystore(nigori) &&
      !nigori.keystore_decryptor_token().blob().empty()) {
    // If the nigori is already migrated and we have pending keys, we might
    // be able to decrypt them using either the keystore decryptor token
    // or the existing keystore keys.
    DecryptPendingKeysWithKeystoreKey(nigori.keystore_decryptor_token(),
                                      cryptographer);
  }

  // Note that triggering migration will have no effect if we're already
  // properly migrated with the newest keystore keys.
  if (ShouldTriggerMigration(nigori, *cryptographer,
                             GetPassphraseType(trans))) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&SyncEncryptionHandlerImpl::RewriteNigori,
                              weak_ptr_factory_.GetWeakPtr()));
  }

  return true;
}

ModelTypeSet SyncEncryptionHandlerImpl::GetEncryptedTypes(
    syncable::BaseTransaction* const trans) const {
  return UnlockVault(trans).encrypted_types;
}

PassphraseType SyncEncryptionHandlerImpl::GetPassphraseType(
    syncable::BaseTransaction* const trans) const {
  return UnlockVault(trans).passphrase_type;
}

Cryptographer* SyncEncryptionHandlerImpl::GetCryptographerUnsafe() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return &vault_unsafe_.cryptographer;
}

ModelTypeSet SyncEncryptionHandlerImpl::GetEncryptedTypesUnsafe() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return vault_unsafe_.encrypted_types;
}

bool SyncEncryptionHandlerImpl::MigratedToKeystore() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ReadTransaction trans(FROM_HERE, user_share_);
  ReadNode nigori_node(&trans);
  if (nigori_node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK)
    return false;
  return IsNigoriMigratedToKeystore(nigori_node.GetNigoriSpecifics());
}

base::Time SyncEncryptionHandlerImpl::migration_time() const {
  return migration_time_;
}

base::Time SyncEncryptionHandlerImpl::custom_passphrase_time() const {
  return custom_passphrase_time_;
}

void SyncEncryptionHandlerImpl::RestoreNigori(
    const SyncEncryptionHandler::NigoriState& nigori_state) {
  DCHECK(thread_checker_.CalledOnValidThread());

  WriteTransaction trans(FROM_HERE, user_share_);

  // Verify we don't already have a nigori node.
  WriteNode nigori_node(&trans);
  BaseNode::InitByLookupResult init_result = nigori_node.InitTypeRoot(NIGORI);
  DCHECK(init_result == BaseNode::INIT_FAILED_ENTRY_NOT_GOOD);

  // Create one.
  syncable::ModelNeutralMutableEntry model_neutral_mutable_entry(
      trans.GetWrappedWriteTrans(), syncable::CREATE_NEW_TYPE_ROOT, NIGORI);
  DCHECK(model_neutral_mutable_entry.good());
  model_neutral_mutable_entry.PutServerIsDir(true);
  model_neutral_mutable_entry.PutUniqueServerTag(ModelTypeToRootTag(NIGORI));
  model_neutral_mutable_entry.PutIsUnsynced(true);

  // Update it with the saved nigori specifics.
  syncable::MutableEntry mutable_entry(trans.GetWrappedWriteTrans(),
                                       syncable::GET_TYPE_ROOT, NIGORI);
  DCHECK(mutable_entry.good());
  sync_pb::EntitySpecifics specifics;
  specifics.mutable_nigori()->CopyFrom(nigori_state.nigori_specifics);
  mutable_entry.PutSpecifics(specifics);

  // Update our state based on the saved nigori node.
  ApplyNigoriUpdate(nigori_state.nigori_specifics, trans.GetWrappedTrans());
}

// This function iterates over all encrypted types.  There are many scenarios in
// which data for some or all types is not currently available.  In that case,
// the lookup of the root node will fail and we will skip encryption for that
// type.
void SyncEncryptionHandlerImpl::ReEncryptEverything(WriteTransaction* trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(UnlockVault(trans->GetWrappedTrans()).cryptographer.is_ready());
  for (ModelTypeSet::Iterator iter =
           UnlockVault(trans->GetWrappedTrans()).encrypted_types.First();
       iter.Good(); iter.Inc()) {
    if (iter.Get() == PASSWORDS || IsControlType(iter.Get()))
      continue;  // These types handle encryption differently.

    ReadNode type_root(trans);
    if (type_root.InitTypeRoot(iter.Get()) != BaseNode::INIT_OK)
      continue;  // Don't try to reencrypt if the type's data is unavailable.

    // Iterate through all children of this datatype.
    base::queue<int64_t> to_visit;
    int64_t child_id = type_root.GetFirstChildId();
    to_visit.push(child_id);
    while (!to_visit.empty()) {
      child_id = to_visit.front();
      to_visit.pop();
      if (child_id == kInvalidId)
        continue;

      WriteNode child(trans);
      if (child.InitByIdLookup(child_id) != BaseNode::INIT_OK)
        continue;  // Possible for locally deleted items.
      if (child.GetIsFolder()) {
        to_visit.push(child.GetFirstChildId());
      }
      if (!child.GetIsPermanentFolder()) {
        // Rewrite the specifics of the node with encrypted data if necessary
        // (only rewrite the non-unique folders).
        child.ResetFromSpecifics();
      }
      to_visit.push(child.GetSuccessorId());
    }
  }

  // Passwords are encrypted with their own legacy scheme.  Passwords are always
  // encrypted so we don't need to check GetEncryptedTypes() here.
  ReadNode passwords_root(trans);
  if (passwords_root.InitTypeRoot(PASSWORDS) == BaseNode::INIT_OK) {
    int64_t child_id = passwords_root.GetFirstChildId();
    while (child_id != kInvalidId) {
      WriteNode child(trans);
      if (child.InitByIdLookup(child_id) != BaseNode::INIT_OK)
        break;  // Possible if we failed to decrypt the data for some reason.
      child.SetPasswordSpecifics(child.GetPasswordSpecifics());
      child_id = child.GetSuccessorId();
    }
  }

  DVLOG(1) << "Re-encrypt everything complete.";

  // NOTE: We notify from within a transaction.
  for (auto& observer : observers_) {
    observer.OnEncryptionComplete();
  }
}

bool SyncEncryptionHandlerImpl::ApplyNigoriUpdateImpl(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Applying nigori node update.";
  bool nigori_types_need_update =
      !UpdateEncryptedTypesFromNigori(nigori, trans);

  if (nigori.custom_passphrase_time() != 0) {
    custom_passphrase_time_ = ProtoTimeToTime(nigori.custom_passphrase_time());
  }
  bool is_nigori_migrated = IsNigoriMigratedToKeystore(nigori);
  PassphraseType* passphrase_type = &UnlockVaultMutable(trans)->passphrase_type;
  if (is_nigori_migrated) {
    migration_time_ = ProtoTimeToTime(nigori.keystore_migration_time());
    PassphraseType nigori_passphrase_type =
        ProtoPassphraseTypeToEnum(nigori.passphrase_type());

    // Only update the local passphrase state if it's a valid transition:
    // - implicit -> keystore
    // - implicit -> frozen implicit
    // - implicit -> custom
    // - keystore -> custom
    // Note: frozen implicit -> custom is not technically a valid transition,
    // but we let it through here as well in case future versions do add support
    // for this transition.
    if (*passphrase_type != nigori_passphrase_type &&
        nigori_passphrase_type != PassphraseType::IMPLICIT_PASSPHRASE &&
        (*passphrase_type == PassphraseType::IMPLICIT_PASSPHRASE ||
         nigori_passphrase_type == PassphraseType::CUSTOM_PASSPHRASE)) {
      DVLOG(1) << "Changing passphrase state from "
               << PassphraseTypeToString(*passphrase_type) << " to "
               << PassphraseTypeToString(nigori_passphrase_type);
      *passphrase_type = nigori_passphrase_type;
      for (auto& observer : observers_) {
        observer.OnPassphraseTypeChanged(
            *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
      }
    }
    if (*passphrase_type == PassphraseType::KEYSTORE_PASSPHRASE &&
        encrypt_everything_) {
      // This is the case where another client that didn't support keystore
      // encryption attempted to enable full encryption. We detect it
      // and switch the passphrase type to frozen implicit passphrase instead
      // due to full encryption not being compatible with keystore passphrase.
      // Because the local passphrase type will not match the nigori passphrase
      // type, we will trigger a rewrite and subsequently a re-migration.
      DVLOG(1) << "Changing passphrase state to FROZEN_IMPLICIT_PASSPHRASE "
               << "due to full encryption.";
      *passphrase_type = PassphraseType::FROZEN_IMPLICIT_PASSPHRASE;
      for (auto& observer : observers_) {
        observer.OnPassphraseTypeChanged(
            *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
      }
    }
  } else {
    // It's possible that while we're waiting for migration a client that does
    // not have keystore encryption enabled switches to a custom passphrase.
    if (nigori.keybag_is_frozen() &&
        *passphrase_type != PassphraseType::CUSTOM_PASSPHRASE) {
      *passphrase_type = PassphraseType::CUSTOM_PASSPHRASE;
      for (auto& observer : observers_) {
        observer.OnPassphraseTypeChanged(
            *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
      }
    }
  }

  Cryptographer* cryptographer = &UnlockVaultMutable(trans)->cryptographer;
  bool nigori_needs_new_keys = false;
  if (!nigori.encryption_keybag().blob().empty()) {
    // We only update the default key if this was a new explicit passphrase.
    // Else, since it was decryptable, it must not have been a new key.
    bool need_new_default_key = false;
    if (is_nigori_migrated) {
      need_new_default_key = IsExplicitPassphrase(
          ProtoPassphraseTypeToEnum(nigori.passphrase_type()));
    } else {
      need_new_default_key = nigori.keybag_is_frozen();
    }
    if (!AttemptToInstallKeybag(nigori.encryption_keybag(),
                                need_new_default_key, cryptographer)) {
      // Check to see if we can decrypt the keybag using the keystore decryptor
      // token.
      cryptographer->SetPendingKeys(nigori.encryption_keybag());
      if (!nigori.keystore_decryptor_token().blob().empty() &&
          !keystore_key_.empty()) {
        if (DecryptPendingKeysWithKeystoreKey(nigori.keystore_decryptor_token(),
                                              cryptographer)) {
          nigori_needs_new_keys =
              cryptographer->KeybagIsStale(nigori.encryption_keybag());
        } else {
          LOG(ERROR) << "Failed to decrypt pending keys using keystore "
                     << "bootstrap key.";
        }
      }
    } else {
      // Keybag was installed. We write back our local keybag into the nigori
      // node if the nigori node's keybag either contains less keys or
      // has a different default key.
      nigori_needs_new_keys =
          cryptographer->KeybagIsStale(nigori.encryption_keybag());
    }
  } else {
    // The nigori node has an empty encryption keybag. Attempt to write our
    // local encryption keys into it.
    LOG(WARNING) << "Nigori had empty encryption keybag.";
    nigori_needs_new_keys = true;
  }

  // If we've completed a sync cycle and the cryptographer isn't ready
  // yet or has pending keys, prompt the user for a passphrase.
  if (cryptographer->has_pending_keys()) {
    DVLOG(1) << "OnPassphraseRequired Sent";
    sync_pb::EncryptedData pending_keys = cryptographer->GetPendingKeys();
    for (auto& observer : observers_) {
      observer.OnPassphraseRequired(REASON_DECRYPTION, pending_keys);
    }
  } else if (!cryptographer->is_ready()) {
    DVLOG(1) << "OnPassphraseRequired sent because cryptographer is not "
             << "ready";
    for (auto& observer : observers_) {
      observer.OnPassphraseRequired(REASON_ENCRYPTION,
                                    sync_pb::EncryptedData());
    }
  }

  // Check if the current local encryption state is stricter/newer than the
  // nigori state. If so, we need to overwrite the nigori node with the local
  // state.
  bool passphrase_type_matches = true;
  if (!is_nigori_migrated) {
    DCHECK(*passphrase_type == PassphraseType::CUSTOM_PASSPHRASE ||
           *passphrase_type == PassphraseType::IMPLICIT_PASSPHRASE);
    passphrase_type_matches =
        nigori.keybag_is_frozen() == IsExplicitPassphrase(*passphrase_type);
  } else {
    passphrase_type_matches =
        (ProtoPassphraseTypeToEnum(nigori.passphrase_type()) ==
         *passphrase_type);
  }
  if (!passphrase_type_matches ||
      nigori.encrypt_everything() != encrypt_everything_ ||
      nigori_types_need_update || nigori_needs_new_keys) {
    DVLOG(1) << "Triggering nigori rewrite.";
    return false;
  }
  return true;
}

void SyncEncryptionHandlerImpl::RewriteNigori() {
  DVLOG(1) << "Writing local encryption state into nigori.";
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  WriteEncryptionStateToNigori(&trans);
}

void SyncEncryptionHandlerImpl::WriteEncryptionStateToNigori(
    WriteTransaction* trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteNode nigori_node(trans);
  // This can happen in tests that don't have nigori nodes.
  if (nigori_node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK)
    return;

  sync_pb::NigoriSpecifics nigori = nigori_node.GetNigoriSpecifics();
  const Cryptographer& cryptographer =
      UnlockVault(trans->GetWrappedTrans()).cryptographer;

  // Will not do anything if we shouldn't or can't migrate. Otherwise
  // migrates, writing the full encryption state as it does.
  if (!AttemptToMigrateNigoriToKeystore(trans, &nigori_node)) {
    if (cryptographer.is_ready() &&
        nigori_overwrite_count_ < kNigoriOverwriteLimit) {
      // Does not modify the encrypted blob if the unencrypted data already
      // matches what is about to be written.
      sync_pb::EncryptedData original_keys = nigori.encryption_keybag();
      if (!cryptographer.GetKeys(nigori.mutable_encryption_keybag()))
        NOTREACHED();

      if (nigori.encryption_keybag().SerializeAsString() !=
          original_keys.SerializeAsString()) {
        // We've updated the nigori node's encryption keys. In order to prevent
        // a possible looping of two clients constantly overwriting each other,
        // we limit the absolute number of overwrites per client instantiation.
        nigori_overwrite_count_++;
        UMA_HISTOGRAM_COUNTS("Sync.AutoNigoriOverwrites",
                             nigori_overwrite_count_);
      }

      // Note: we don't try to set keybag_is_frozen here since if that
      // is lost the user can always set it again (and we don't want to clobber
      // any migration state). The main goal at this point is to preserve
      // the encryption keys so all data remains decryptable.
    }
    syncable::UpdateNigoriFromEncryptedTypes(
        UnlockVault(trans->GetWrappedTrans()).encrypted_types,
        encrypt_everything_, &nigori);
    if (!custom_passphrase_time_.is_null()) {
      nigori.set_custom_passphrase_time(
          TimeToProtoTime(custom_passphrase_time_));
    }

    // If nothing has changed, this is a no-op.
    nigori_node.SetNigoriSpecifics(nigori);
  }
}

bool SyncEncryptionHandlerImpl::UpdateEncryptedTypesFromNigori(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ModelTypeSet* encrypted_types = &UnlockVaultMutable(trans)->encrypted_types;
  if (nigori.encrypt_everything()) {
    EnableEncryptEverythingImpl(trans);
    DCHECK(*encrypted_types == EncryptableUserTypes());
    return true;
  } else if (encrypt_everything_) {
    DCHECK(*encrypted_types == EncryptableUserTypes());
    return false;
  }

  ModelTypeSet nigori_encrypted_types;
  nigori_encrypted_types = syncable::GetEncryptedTypesFromNigori(nigori);
  nigori_encrypted_types.PutAll(SensitiveTypes());

  // If anything more than the sensitive types were encrypted, and
  // encrypt_everything is not explicitly set to false, we assume it means
  // a client intended to enable encrypt everything.
  if (!nigori.has_encrypt_everything() &&
      !Difference(nigori_encrypted_types, SensitiveTypes()).Empty()) {
    if (!encrypt_everything_) {
      encrypt_everything_ = true;
      *encrypted_types = EncryptableUserTypes();
      for (auto& observer : observers_) {
        observer.OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_);
      }
    }
    DCHECK(*encrypted_types == EncryptableUserTypes());
    return false;
  }

  MergeEncryptedTypes(nigori_encrypted_types, trans);
  return *encrypted_types == nigori_encrypted_types;
}

void SyncEncryptionHandlerImpl::SetCustomPassphrase(
    const std::string& passphrase,
    WriteTransaction* trans,
    WriteNode* nigori_node) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsNigoriMigratedToKeystore(nigori_node->GetNigoriSpecifics()));
  KeyParams key_params = {"localhost", "dummy", passphrase};

  if (GetPassphraseType(trans->GetWrappedTrans()) !=
      PassphraseType::KEYSTORE_PASSPHRASE) {
    DVLOG(1) << "Failing to set a custom passphrase because one has already "
             << "been set.";
    FinishSetPassphrase(false, std::string(), trans, nigori_node);
    return;
  }

  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans->GetWrappedTrans())->cryptographer;
  if (cryptographer->has_pending_keys()) {
    // This theoretically shouldn't happen, because the only way to have pending
    // keys after migrating to keystore support is if a custom passphrase was
    // set, which should update passpshrase_state_ and should be caught by the
    // if statement above. For the sake of safety though, we check for it in
    // case a client is misbehaving.
    LOG(ERROR) << "Failing to set custom passphrase because of pending keys.";
    FinishSetPassphrase(false, std::string(), trans, nigori_node);
    return;
  }

  std::string bootstrap_token;
  if (!cryptographer->AddKey(key_params)) {
    NOTREACHED() << "Failed to add key to cryptographer.";
    return;
  }

  DVLOG(1) << "Setting custom passphrase.";
  cryptographer->GetBootstrapToken(&bootstrap_token);
  PassphraseType* passphrase_type =
      &UnlockVaultMutable(trans->GetWrappedTrans())->passphrase_type;
  *passphrase_type = PassphraseType::CUSTOM_PASSPHRASE;
  custom_passphrase_time_ = base::Time::Now();
  for (auto& observer : observers_) {
    observer.OnPassphraseTypeChanged(
        *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
  }
  FinishSetPassphrase(true, bootstrap_token, trans, nigori_node);
}

void SyncEncryptionHandlerImpl::NotifyObserversOfLocalCustomPassphrase(
    WriteTransaction* trans) {
  WriteNode nigori_node(trans);
  BaseNode::InitByLookupResult init_result = nigori_node.InitTypeRoot(NIGORI);
  DCHECK_EQ(init_result, BaseNode::INIT_OK);
  NigoriState nigori_state;
  nigori_state.nigori_specifics = nigori_node.GetNigoriSpecifics();
  DCHECK(nigori_state.nigori_specifics.passphrase_type() ==
             sync_pb::NigoriSpecifics::CUSTOM_PASSPHRASE ||
         nigori_state.nigori_specifics.passphrase_type() ==
             sync_pb::NigoriSpecifics::FROZEN_IMPLICIT_PASSPHRASE);
  for (auto& observer : observers_) {
    observer.OnLocalSetPassphraseEncryption(nigori_state);
  }
}

void SyncEncryptionHandlerImpl::DecryptPendingKeysWithExplicitPassphrase(
    const std::string& passphrase,
    WriteTransaction* trans,
    WriteNode* nigori_node) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsExplicitPassphrase(GetPassphraseType(trans->GetWrappedTrans())));
  KeyParams key_params = {"localhost", "dummy", passphrase};

  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans->GetWrappedTrans())->cryptographer;
  if (!cryptographer->has_pending_keys()) {
    // Note that this *can* happen in a rare situation where data is
    // re-encrypted on another client while a SetDecryptionPassphrase() call is
    // in-flight on this client. It is rare enough that we choose to do nothing.
    NOTREACHED() << "Attempt to set decryption passphrase failed because there "
                 << "were no pending keys.";
    return;
  }

  DCHECK(IsExplicitPassphrase(GetPassphraseType(trans->GetWrappedTrans())));
  bool success = false;
  std::string bootstrap_token;
  if (cryptographer->DecryptPendingKeys(key_params)) {
    DVLOG(1) << "Explicit passphrase accepted for decryption.";
    cryptographer->GetBootstrapToken(&bootstrap_token);
    success = true;
  } else {
    DVLOG(1) << "Explicit passphrase failed to decrypt.";
    success = false;
  }
  if (success && !keystore_key_.empty()) {
    // Should already be part of the encryption keybag, but we add it just
    // in case.
    KeyParams key_params = {"localhost", "dummy", keystore_key_};
    cryptographer->AddNonDefaultKey(key_params);
  }
  FinishSetPassphrase(success, bootstrap_token, trans, nigori_node);
}

void SyncEncryptionHandlerImpl::FinishSetPassphrase(
    bool success,
    const std::string& bootstrap_token,
    WriteTransaction* trans,
    WriteNode* nigori_node) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (auto& observer : observers_) {
    observer.OnCryptographerStateChanged(
        &UnlockVaultMutable(trans->GetWrappedTrans())->cryptographer);
  }

  // It's possible we need to change the bootstrap token even if we failed to
  // set the passphrase (for example if we need to preserve the new GAIA
  // passphrase).
  if (!bootstrap_token.empty()) {
    DVLOG(1) << "Passphrase bootstrap token updated.";
    for (auto& observer : observers_) {
      observer.OnBootstrapTokenUpdated(bootstrap_token,
                                       PASSPHRASE_BOOTSTRAP_TOKEN);
    }
  }

  const Cryptographer& cryptographer =
      UnlockVault(trans->GetWrappedTrans()).cryptographer;
  if (!success) {
    if (cryptographer.is_ready()) {
      LOG(ERROR) << "Attempt to change passphrase failed while cryptographer "
                 << "was ready.";
    } else if (cryptographer.has_pending_keys()) {
      for (auto& observer : observers_) {
        observer.OnPassphraseRequired(REASON_DECRYPTION,
                                      cryptographer.GetPendingKeys());
      }
    } else {
      for (auto& observer : observers_) {
        observer.OnPassphraseRequired(REASON_ENCRYPTION,
                                      sync_pb::EncryptedData());
      }
    }
    return;
  }
  DCHECK(success);
  DCHECK(cryptographer.is_ready());

  // Will do nothing if we're already properly migrated or unable to migrate
  // (in otherwords, if ShouldTriggerMigration is false).
  // Otherwise will update the nigori node with the current migrated state,
  // writing all encryption state as it does.
  if (!AttemptToMigrateNigoriToKeystore(trans, nigori_node)) {
    sync_pb::NigoriSpecifics nigori(nigori_node->GetNigoriSpecifics());
    // Does not modify nigori.encryption_keybag() if the original decrypted
    // data was the same.
    if (!cryptographer.GetKeys(nigori.mutable_encryption_keybag()))
      NOTREACHED();
    if (IsNigoriMigratedToKeystore(nigori)) {
      DCHECK(keystore_key_.empty() ||
             IsExplicitPassphrase(GetPassphraseType(trans->GetWrappedTrans())));
      DVLOG(1) << "Leaving nigori migration state untouched after setting"
               << " passphrase.";
    } else {
      nigori.set_keybag_is_frozen(
          IsExplicitPassphrase(GetPassphraseType(trans->GetWrappedTrans())));
    }
    // If we set a new custom passphrase, store the timestamp.
    if (!custom_passphrase_time_.is_null()) {
      nigori.set_custom_passphrase_time(
          TimeToProtoTime(custom_passphrase_time_));
    }
    nigori_node->SetNigoriSpecifics(nigori);
  }

  // Must do this after OnPassphraseTypeChanged, in order to ensure the PSS
  // checks the passphrase state after it has been set.
  for (auto& observer : observers_) {
    observer.OnPassphraseAccepted();
  }

  // Does nothing if everything is already encrypted.
  // TODO(zea): If we just migrated and enabled encryption, this will be
  // redundant. Figure out a way to not do this unnecessarily.
  ReEncryptEverything(trans);
}

void SyncEncryptionHandlerImpl::MergeEncryptedTypes(
    ModelTypeSet new_encrypted_types,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Only UserTypes may be encrypted.
  DCHECK(EncryptableUserTypes().HasAll(new_encrypted_types));

  ModelTypeSet* encrypted_types = &UnlockVaultMutable(trans)->encrypted_types;
  if (!encrypted_types->HasAll(new_encrypted_types)) {
    *encrypted_types = new_encrypted_types;
    for (auto& observer : observers_) {
      observer.OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_);
    }
  }
}

SyncEncryptionHandlerImpl::Vault* SyncEncryptionHandlerImpl::UnlockVaultMutable(
    syncable::BaseTransaction* const trans) {
  DCHECK_EQ(user_share_->directory.get(), trans->directory());
  return &vault_unsafe_;
}

const SyncEncryptionHandlerImpl::Vault& SyncEncryptionHandlerImpl::UnlockVault(
    syncable::BaseTransaction* const trans) const {
  DCHECK_EQ(user_share_->directory.get(), trans->directory());
  return vault_unsafe_;
}

bool SyncEncryptionHandlerImpl::ShouldTriggerMigration(
    const sync_pb::NigoriSpecifics& nigori,
    const Cryptographer& cryptographer,
    PassphraseType passphrase_type) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Don't migrate if there are pending encryption keys (because data
  // encrypted with the pending keys will not be decryptable).
  if (cryptographer.has_pending_keys())
    return false;
  if (IsNigoriMigratedToKeystore(nigori)) {
    // If the nigori is already migrated but does not reflect the explicit
    // passphrase state, remigrate. Similarly, if the nigori has an explicit
    // passphrase but does not have full encryption, or the nigori has an
    // implicit passphrase but does have full encryption, re-migrate.
    // Note that this is to defend against other clients without keystore
    // encryption enabled transitioning to states that are no longer valid.
    if (passphrase_type != PassphraseType::KEYSTORE_PASSPHRASE &&
        nigori.passphrase_type() ==
            sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE) {
      return true;
    } else if (IsExplicitPassphrase(passphrase_type) && !encrypt_everything_) {
      return true;
    } else if (passphrase_type == PassphraseType::KEYSTORE_PASSPHRASE &&
               encrypt_everything_) {
      return true;
    } else if (cryptographer.is_ready() &&
               !cryptographer.CanDecryptUsingDefaultKey(
                   nigori.encryption_keybag())) {
      // We need to overwrite the keybag. This might involve overwriting the
      // keystore decryptor too.
      return true;
    } else if (old_keystore_keys_.size() > 0 && !keystore_key_.empty()) {
      // Check to see if a server key rotation has happened, but the nigori
      // node's keys haven't been rotated yet, and hence we should re-migrate.
      // Note that once a key rotation has been performed, we no longer
      // preserve backwards compatibility, and the keybag will therefore be
      // encrypted with the current keystore key.
      Cryptographer temp_cryptographer(cryptographer.encryptor());
      KeyParams keystore_params = {"localhost", "dummy", keystore_key_};
      temp_cryptographer.AddKey(keystore_params);
      if (!temp_cryptographer.CanDecryptUsingDefaultKey(
              nigori.encryption_keybag())) {
        return true;
      }
    }
    return false;
  } else if (keystore_key_.empty()) {
    // If we haven't already migrated, we don't want to do anything unless
    // a keystore key is available (so that those clients without keystore
    // encryption enabled aren't forced into new states, e.g. frozen implicit
    // passphrase).
    return false;
  }
  return true;
}

bool SyncEncryptionHandlerImpl::AttemptToMigrateNigoriToKeystore(
    WriteTransaction* trans,
    WriteNode* nigori_node) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const sync_pb::NigoriSpecifics& old_nigori =
      nigori_node->GetNigoriSpecifics();
  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans->GetWrappedTrans())->cryptographer;
  PassphraseType* passphrase_type =
      &UnlockVaultMutable(trans->GetWrappedTrans())->passphrase_type;
  if (!ShouldTriggerMigration(old_nigori, *cryptographer, *passphrase_type))
    return false;

  DVLOG(1) << "Starting nigori migration to keystore support.";
  sync_pb::NigoriSpecifics migrated_nigori(old_nigori);

  PassphraseType new_passphrase_type =
      GetPassphraseType(trans->GetWrappedTrans());
  bool new_encrypt_everything = encrypt_everything_;
  if (encrypt_everything_ && !IsExplicitPassphrase(*passphrase_type)) {
    DVLOG(1) << "Switching to frozen implicit passphrase due to already having "
             << "full encryption.";
    new_passphrase_type = PassphraseType::FROZEN_IMPLICIT_PASSPHRASE;
    migrated_nigori.clear_keystore_decryptor_token();
  } else if (IsExplicitPassphrase(*passphrase_type)) {
    DVLOG_IF(1, !encrypt_everything_) << "Enabling encrypt everything due to "
                                      << "explicit passphrase";
    new_encrypt_everything = true;
    migrated_nigori.clear_keystore_decryptor_token();
  } else {
    DCHECK(!encrypt_everything_);
    new_passphrase_type = PassphraseType::KEYSTORE_PASSPHRASE;
    DVLOG(1) << "Switching to keystore passphrase state.";
  }
  migrated_nigori.set_encrypt_everything(new_encrypt_everything);
  migrated_nigori.set_passphrase_type(
      EnumPassphraseTypeToProto(new_passphrase_type));
  migrated_nigori.set_keybag_is_frozen(true);

  if (!keystore_key_.empty()) {
    KeyParams key_params = {"localhost", "dummy", keystore_key_};
    if ((old_keystore_keys_.size() > 0 &&
         new_passphrase_type == PassphraseType::KEYSTORE_PASSPHRASE) ||
        !cryptographer->is_initialized()) {
      // Either at least one key rotation has been performed, so we no longer
      // care about backwards compatibility, or we're generating keystore-based
      // encryption keys without knowing the GAIA password (and therefore the
      // cryptographer is not initialized), so we can't support backwards
      // compatibility. Ensure the keystore key is the default key.
      DVLOG(1) << "Migrating keybag to keystore key.";
      bool cryptographer_was_ready = cryptographer->is_ready();
      if (!cryptographer->AddKey(key_params)) {
        LOG(ERROR) << "Failed to add keystore key as default key";
        UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                  FAILED_TO_SET_DEFAULT_KEYSTORE,
                                  MIGRATION_RESULT_SIZE);
        return false;
      }
      if (!cryptographer_was_ready && cryptographer->is_ready()) {
        for (auto& observer : observers_) {
          observer.OnPassphraseAccepted();
        }
      }
    } else {
      // We're in backwards compatible mode -- either the account has an
      // explicit passphrase, or we want to preserve the current GAIA-based key
      // as the default because we can (there have been no key rotations since
      // the migration).
      DVLOG(1) << "Migrating keybag while preserving old key";
      if (!cryptographer->AddNonDefaultKey(key_params)) {
        LOG(ERROR) << "Failed to add keystore key as non-default key.";
        UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                  FAILED_TO_SET_NONDEFAULT_KEYSTORE,
                                  MIGRATION_RESULT_SIZE);
        return false;
      }
    }
  }
  if (!old_keystore_keys_.empty()) {
    // Go through and add all the old keystore keys as non default keys, so
    // they'll be preserved in the encryption_keybag when we next write the
    // nigori node.
    for (std::vector<std::string>::const_iterator iter =
             old_keystore_keys_.begin();
         iter != old_keystore_keys_.end(); ++iter) {
      KeyParams key_params = {"localhost", "dummy", *iter};
      cryptographer->AddNonDefaultKey(key_params);
    }
  }
  if (new_passphrase_type == PassphraseType::KEYSTORE_PASSPHRASE &&
      !GetKeystoreDecryptor(
          *cryptographer, keystore_key_,
          migrated_nigori.mutable_keystore_decryptor_token())) {
    LOG(ERROR) << "Failed to extract keystore decryptor token.";
    UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                              FAILED_TO_EXTRACT_DECRYPTOR,
                              MIGRATION_RESULT_SIZE);
    return false;
  }
  if (!cryptographer->GetKeys(migrated_nigori.mutable_encryption_keybag())) {
    LOG(ERROR) << "Failed to extract encryption keybag.";
    UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                              FAILED_TO_EXTRACT_KEYBAG, MIGRATION_RESULT_SIZE);
    return false;
  }

  if (migration_time_.is_null())
    migration_time_ = base::Time::Now();
  migrated_nigori.set_keystore_migration_time(TimeToProtoTime(migration_time_));

  if (!custom_passphrase_time_.is_null()) {
    migrated_nigori.set_custom_passphrase_time(
        TimeToProtoTime(custom_passphrase_time_));
  }

  for (auto& observer : observers_) {
    observer.OnCryptographerStateChanged(cryptographer);
  }
  if (*passphrase_type != new_passphrase_type) {
    *passphrase_type = new_passphrase_type;
    for (auto& observer : observers_) {
      observer.OnPassphraseTypeChanged(
          *passphrase_type, GetExplicitPassphraseTime(*passphrase_type));
    }
  }

  if (new_encrypt_everything && !encrypt_everything_) {
    EnableEncryptEverythingImpl(trans->GetWrappedTrans());
    ReEncryptEverything(trans);
  } else if (!cryptographer->CanDecryptUsingDefaultKey(
                 old_nigori.encryption_keybag())) {
    DVLOG(1) << "Rencrypting everything due to key rotation.";
    ReEncryptEverything(trans);
  }

  DVLOG(1) << "Completing nigori migration to keystore support.";
  nigori_node->SetNigoriSpecifics(migrated_nigori);

  if (new_encrypt_everything &&
      (new_passphrase_type == PassphraseType::FROZEN_IMPLICIT_PASSPHRASE ||
       new_passphrase_type == PassphraseType::CUSTOM_PASSPHRASE)) {
    NotifyObserversOfLocalCustomPassphrase(trans);
  }

  switch (new_passphrase_type) {
    case PassphraseType::KEYSTORE_PASSPHRASE:
      if (old_keystore_keys_.size() > 0) {
        UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                  MIGRATION_SUCCESS_KEYSTORE_NONDEFAULT,
                                  MIGRATION_RESULT_SIZE);
      } else {
        UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                  MIGRATION_SUCCESS_KEYSTORE_DEFAULT,
                                  MIGRATION_RESULT_SIZE);
      }
      break;
    case PassphraseType::FROZEN_IMPLICIT_PASSPHRASE:
      UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                MIGRATION_SUCCESS_FROZEN_IMPLICIT,
                                MIGRATION_RESULT_SIZE);
      break;
    case PassphraseType::CUSTOM_PASSPHRASE:
      UMA_HISTOGRAM_ENUMERATION("Sync.AttemptNigoriMigration",
                                MIGRATION_SUCCESS_CUSTOM,
                                MIGRATION_RESULT_SIZE);
      break;
    default:
      NOTREACHED();
      break;
  }
  return true;
}

bool SyncEncryptionHandlerImpl::GetKeystoreDecryptor(
    const Cryptographer& cryptographer,
    const std::string& keystore_key,
    sync_pb::EncryptedData* encrypted_blob) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!keystore_key.empty());
  DCHECK(cryptographer.is_ready());
  std::string serialized_nigori;
  serialized_nigori = cryptographer.GetDefaultNigoriKeyData();
  if (serialized_nigori.empty()) {
    LOG(ERROR) << "Failed to get cryptographer bootstrap token.";
    return false;
  }
  Cryptographer temp_cryptographer(cryptographer.encryptor());
  KeyParams key_params = {"localhost", "dummy", keystore_key};
  if (!temp_cryptographer.AddKey(key_params))
    return false;
  if (!temp_cryptographer.EncryptString(serialized_nigori, encrypted_blob))
    return false;
  return true;
}

bool SyncEncryptionHandlerImpl::AttemptToInstallKeybag(
    const sync_pb::EncryptedData& keybag,
    bool update_default,
    Cryptographer* cryptographer) {
  if (!cryptographer->CanDecrypt(keybag))
    return false;
  cryptographer->InstallKeys(keybag);
  if (update_default)
    cryptographer->SetDefaultKey(keybag.key_name());
  return true;
}

void SyncEncryptionHandlerImpl::EnableEncryptEverythingImpl(
    syncable::BaseTransaction* const trans) {
  ModelTypeSet* encrypted_types = &UnlockVaultMutable(trans)->encrypted_types;
  if (encrypt_everything_) {
    DCHECK_EQ(EncryptableUserTypes(), *encrypted_types);
    return;
  }
  encrypt_everything_ = true;
  *encrypted_types = EncryptableUserTypes();
  for (auto& observer : observers_) {
    observer.OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_);
  }
}

bool SyncEncryptionHandlerImpl::DecryptPendingKeysWithKeystoreKey(
    const sync_pb::EncryptedData& keystore_decryptor_token,
    Cryptographer* cryptographer) {
  DCHECK(cryptographer->has_pending_keys());
  if (keystore_decryptor_token.blob().empty())
    return false;
  Cryptographer temp_cryptographer(cryptographer->encryptor());

  // First, go through and all all the old keystore keys to the temporary
  // cryptographer.
  for (size_t i = 0; i < old_keystore_keys_.size(); ++i) {
    KeyParams old_key_params = {"localhost", "dummy", old_keystore_keys_[i]};
    temp_cryptographer.AddKey(old_key_params);
  }

  // Then add the current keystore key as the default key and see if we can
  // decrypt.
  KeyParams keystore_params = {"localhost", "dummy", keystore_key_};
  if (temp_cryptographer.AddKey(keystore_params) &&
      temp_cryptographer.CanDecrypt(keystore_decryptor_token)) {
    // Someone else migrated the nigori for us! How generous! Go ahead and
    // install both the keystore key and the new default encryption key
    // (i.e. the one provided by the keystore decryptor token) into the
    // cryptographer.
    // The keystore decryptor token is a keystore key encrypted blob containing
    // the current serialized default encryption key (and as such should be
    // able to decrypt the nigori node's encryption keybag).
    // Note: it's possible a key rotation has happened since the migration, and
    // we're decrypting using an old keystore key. In that case we need to
    // ensure we re-encrypt using the newest key.
    DVLOG(1) << "Attempting to decrypt pending keys using "
             << "keystore decryptor token.";
    std::string serialized_nigori =
        temp_cryptographer.DecryptToString(keystore_decryptor_token);

    // This will decrypt the pending keys and add them if possible. The key
    // within |serialized_nigori| will be the default after.
    cryptographer->ImportNigoriKey(serialized_nigori);

    if (!temp_cryptographer.CanDecryptUsingDefaultKey(
            keystore_decryptor_token)) {
      // The keystore decryptor token was derived from an old keystore key.
      // A key rotation is necessary, so set the current keystore key as the
      // default key (which will trigger a re-migration).
      DVLOG(1) << "Pending keys based on old keystore key. Setting newest "
               << "keystore key as default.";
      cryptographer->AddKey(keystore_params);
    } else {
      // Theoretically the encryption keybag should already contain the keystore
      // key. We explicitly add it as a safety measure.
      DVLOG(1) << "Pending keys based on newest keystore key.";
      cryptographer->AddNonDefaultKey(keystore_params);
    }
    if (cryptographer->is_ready()) {
      std::string bootstrap_token;
      cryptographer->GetBootstrapToken(&bootstrap_token);
      DVLOG(1) << "Keystore decryptor token decrypted pending keys.";
      // Note: These are separate loops to match previous functionality and not
      // out of explicit knowledge that they must be.
      for (auto& observer : observers_) {
        observer.OnPassphraseAccepted();
      }
      for (auto& observer : observers_) {
        observer.OnBootstrapTokenUpdated(bootstrap_token,
                                         PASSPHRASE_BOOTSTRAP_TOKEN);
      }
      for (auto& observer : observers_) {
        observer.OnCryptographerStateChanged(cryptographer);
      }
      return true;
    }
  }
  return false;
}

base::Time SyncEncryptionHandlerImpl::GetExplicitPassphraseTime(
    PassphraseType passphrase_type) const {
  if (passphrase_type == PassphraseType::FROZEN_IMPLICIT_PASSPHRASE)
    return migration_time();
  else if (passphrase_type == PassphraseType::CUSTOM_PASSPHRASE)
    return custom_passphrase_time();
  return base::Time();
}

}  // namespace syncer
