// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_CRYPTOGRAPHER_H_
#define COMPONENTS_SYNC_BASE_CRYPTOGRAPHER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "components/sync/base/nigori.h"
#include "components/sync/protocol/encryption.pb.h"

namespace sync_pb {
class NigoriKeyBag;
}

namespace syncer {

class Encryptor;

extern const char kNigoriTag[];

// The parameters used to initialize a Nigori instance.
struct KeyParams {
  std::string hostname;
  std::string username;
  std::string password;
};

// This class manages the Nigori objects used to encrypt and decrypt sensitive
// sync data (eg. passwords). Each Nigori object knows how to handle data
// protected with a particular passphrase.
//
// Whenever an update to the Nigori sync node is received from the server,
// SetPendingKeys should be called with the encrypted contents of that node.
// Most likely, an updated Nigori node means that a new passphrase has been set
// and that future node updates won't be decryptable. To remedy this, the user
// should be prompted for the new passphrase and DecryptPendingKeys be called.
//
// Whenever a update to an encrypted node is received from the server,
// CanDecrypt should be used to verify whether the Cryptographer can decrypt
// that node. If it cannot, then the application of that update should be
// delayed until after it can be decrypted.
class Cryptographer {
 public:
  // Does not take ownership of |encryptor|.
  explicit Cryptographer(Encryptor* encryptor);
  explicit Cryptographer(const Cryptographer& other);
  ~Cryptographer();

  // |restored_bootstrap_token| can be provided via this method to bootstrap
  // Cryptographer instance into the ready state (is_ready will be true).
  // It must be a string that was previously built by the
  // GetSerializedBootstrapToken function.  It is possible that the token is no
  // longer valid (due to server key change), in which case the normal
  // decryption code paths will fail and the user will need to provide a new
  // passphrase.
  // It is an error to call this if is_ready() == true, though it is fair to
  // never call Bootstrap at all.
  void Bootstrap(const std::string& restored_bootstrap_token);

  // Returns whether we can decrypt |encrypted| using the keys we currently know
  // about.
  bool CanDecrypt(const sync_pb::EncryptedData& encrypted) const;

  // Returns whether |encrypted| can be decrypted using the default encryption
  // key.
  bool CanDecryptUsingDefaultKey(const sync_pb::EncryptedData& encrypted) const;

  // Encrypts |message| into |encrypted|. Does not overwrite |encrypted| if
  // |message| already matches the decrypted data within |encrypted| and
  // |encrypted| was encrypted with the current default key. This avoids
  // unnecessarily modifying |encrypted| if the change had no practical effect.
  // Returns true unless encryption fails or |message| isn't valid (e.g. a
  // required field isn't set).
  bool Encrypt(const ::google::protobuf::MessageLite& message,
               sync_pb::EncryptedData* encrypted) const;

  // Encrypted |serialized| into |encrypted|. Does not overwrite |encrypted| if
  // |message| already matches the decrypted data within |encrypted| and
  // |encrypted| was encrypted with the current default key. This avoids
  // unnecessarily modifying |encrypted| if the change had no practical effect.
  // Returns true unless encryption fails or |message| isn't valid (e.g. a
  // required field isn't set).
  bool EncryptString(const std::string& serialized,
                     sync_pb::EncryptedData* encrypted) const;

  // Decrypts |encrypted| into |message|. Returns true unless decryption fails,
  // or |message| fails to parse the decrypted data.
  bool Decrypt(const sync_pb::EncryptedData& encrypted,
               ::google::protobuf::MessageLite* message) const;

  // Decrypts |encrypted| and returns plaintext decrypted data. If decryption
  // fails, returns empty string.
  std::string DecryptToString(const sync_pb::EncryptedData& encrypted) const;

  // Encrypts the set of currently known keys into |encrypted|. Returns true if
  // successful.
  bool GetKeys(sync_pb::EncryptedData* encrypted) const;

  // Creates a new Nigori instance using |params|. If successful, |params| will
  // become the default encryption key and be used for all future calls to
  // Encrypt.
  // Will decrypt the pending keys and install them if possible (pending key
  // will not overwrite default).
  bool AddKey(const KeyParams& params);

  // Same as AddKey(..), but builds the new Nigori from a previously persisted
  // bootstrap token. This can be useful when consuming a bootstrap token
  // with a cryptographer that has already been initialized.
  // Updates the default key.
  // Will decrypt the pending keys and install them if possible (pending key
  // will not overwrite default).
  bool AddKeyFromBootstrapToken(const std::string& restored_bootstrap_token);

  // Creates a new Nigori instance using |params|. If successful, |params|
  // will be added to the nigori keybag, but will not be the default encryption
  // key (default_nigori_ will remain the same).
  // Prereq: is_initialized() must be true.
  // Will decrypt the pending keys and install them if possible (pending key
  // will become the new default).
  bool AddNonDefaultKey(const KeyParams& params);

  // Decrypts |encrypted| and uses its contents to initialize Nigori instances.
  // Returns true unless decryption of |encrypted| fails. The caller is
  // responsible for checking that CanDecrypt(encrypted) == true.
  // Does not modify the default key.
  void InstallKeys(const sync_pb::EncryptedData& encrypted);

  // Makes a local copy of |encrypted| to later be decrypted by
  // DecryptPendingKeys. This should only be used if CanDecrypt(encrypted) ==
  // false.
  void SetPendingKeys(const sync_pb::EncryptedData& encrypted);

  // Makes |pending_keys_| available to callers that may want to cache its
  // value for later use on the UI thread. It is illegal to call this if the
  // cryptographer has no pending keys. Like other calls that access the
  // cryptographer, this method must be called from within a transaction.
  const sync_pb::EncryptedData& GetPendingKeys() const;

  // Attempts to decrypt the set of keys that was copied in the previous call to
  // SetPendingKeys using |params|. Returns true if the pending keys were
  // successfully decrypted and installed. If successful, the default key
  // is updated.
  bool DecryptPendingKeys(const KeyParams& params);

  // Sets the default key to the nigori with name |key_name|. |key_name| must
  // correspond to a nigori that has already been installed into the keybag.
  void SetDefaultKey(const std::string& key_name);

  bool is_initialized() const {
    return !nigoris_.empty() && !default_nigori_name_.empty();
  }

  // Returns whether this Cryptographer is ready to encrypt and decrypt data.
  bool is_ready() const { return is_initialized() && !has_pending_keys(); }

  // Returns whether there is a pending set of keys that needs to be decrypted.
  bool has_pending_keys() const { return nullptr != pending_keys_.get(); }

  // Obtain a token that can be provided on construction to a future
  // Cryptographer instance to bootstrap itself.  Returns false if such a token
  // can't be created (i.e. if this Cryptograhper doesn't have valid keys).
  bool GetBootstrapToken(std::string* token) const;

  Encryptor* encryptor() const { return encryptor_; }

  // Returns true if |keybag| is decryptable and either is a subset of nigoris_
  // and/or has a different default key.
  bool KeybagIsStale(const sync_pb::EncryptedData& keybag) const;

  // Returns the name of the Nigori key currently used for encryption.
  std::string GetDefaultNigoriKeyName() const;

  // Returns a serialized sync_pb::NigoriKey version of current default
  // encryption key.
  std::string GetDefaultNigoriKeyData() const;

  // Generates a new Nigori from |serialized_nigori_key|, and if successful
  // installs the new nigori as the default key.
  bool ImportNigoriKey(const std::string& serialized_nigori_key);

 private:
  using NigoriMap = std::map<std::string, std::unique_ptr<const Nigori>>;

  // Helper method to instantiate Nigori instances for each set of key
  // parameters in |bag|.
  // Does not update the default nigori.
  void InstallKeyBag(const sync_pb::NigoriKeyBag& bag);

  // Helper method to add a nigori to the keybag, optionally making it the
  // default as well.
  bool AddKeyImpl(std::unique_ptr<Nigori> nigori, bool set_as_default);

  // Helper to unencrypt a bootstrap token into a serialized sync_pb::NigoriKey.
  std::string UnpackBootstrapToken(const std::string& token) const;

  Encryptor* const encryptor_;

  // The Nigoris we know about, mapped by key name.
  NigoriMap nigoris_;

  // The key name associated with the default nigori. If non-empty, must
  // correspond to a nigori within |nigoris_|.
  std::string default_nigori_name_;

  std::unique_ptr<sync_pb::EncryptedData> pending_keys_;

  DISALLOW_ASSIGN(Cryptographer);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_CRYPTOGRAPHER_H_
