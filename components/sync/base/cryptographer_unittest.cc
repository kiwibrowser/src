// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/cryptographer.h"

#include "base/strings/string_util.h"
#include "components/sync/base/fake_encryptor.h"
#include "components/sync/protocol/password_specifics.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using ::testing::_;

}  // namespace

class CryptographerTest : public ::testing::Test {
 protected:
  CryptographerTest() : cryptographer_(&encryptor_) {}

  FakeEncryptor encryptor_;
  Cryptographer cryptographer_;
};

TEST_F(CryptographerTest, EmptyCantDecrypt) {
  EXPECT_FALSE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted;
  encrypted.set_key_name("foo");
  encrypted.set_blob("bar");

  EXPECT_FALSE(cryptographer_.CanDecrypt(encrypted));
}

TEST_F(CryptographerTest, EmptyCantEncrypt) {
  EXPECT_FALSE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted;
  sync_pb::PasswordSpecificsData original;
  EXPECT_FALSE(cryptographer_.Encrypt(original, &encrypted));
}

TEST_F(CryptographerTest, MissingCantDecrypt) {
  KeyParams params = {"localhost", "dummy", "dummy"};
  cryptographer_.AddKey(params);
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted;
  encrypted.set_key_name("foo");
  encrypted.set_blob("bar");

  EXPECT_FALSE(cryptographer_.CanDecrypt(encrypted));
}

TEST_F(CryptographerTest, CanEncryptAndDecrypt) {
  KeyParams params = {"localhost", "dummy", "dummy"};
  EXPECT_TRUE(cryptographer_.AddKey(params));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("azure");
  original.set_password_value("hunter2");

  sync_pb::EncryptedData encrypted;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted));

  sync_pb::PasswordSpecificsData decrypted;
  EXPECT_TRUE(cryptographer_.Decrypt(encrypted, &decrypted));

  EXPECT_EQ(original.SerializeAsString(), decrypted.SerializeAsString());
}

TEST_F(CryptographerTest, EncryptOnlyIfDifferent) {
  KeyParams params = {"localhost", "dummy", "dummy"};
  EXPECT_TRUE(cryptographer_.AddKey(params));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("azure");
  original.set_password_value("hunter2");

  sync_pb::EncryptedData encrypted;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted));

  sync_pb::EncryptedData encrypted2, encrypted3;
  encrypted2.CopyFrom(encrypted);
  encrypted3.CopyFrom(encrypted);
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted2));

  // Now encrypt with a new default key. Should overwrite the old data.
  KeyParams params_new = {"localhost", "dummy", "dummy2"};
  cryptographer_.AddKey(params_new);
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted3));

  sync_pb::PasswordSpecificsData decrypted;
  EXPECT_TRUE(cryptographer_.Decrypt(encrypted2, &decrypted));
  // encrypted2 should match encrypted, encrypted3 should not (due to salting).
  EXPECT_EQ(encrypted.SerializeAsString(), encrypted2.SerializeAsString());
  EXPECT_NE(encrypted.SerializeAsString(), encrypted3.SerializeAsString());
  EXPECT_EQ(original.SerializeAsString(), decrypted.SerializeAsString());
}

TEST_F(CryptographerTest, AddKeySetsDefault) {
  KeyParams params1 = {"localhost", "dummy", "dummy1"};
  EXPECT_TRUE(cryptographer_.AddKey(params1));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("azure");
  original.set_password_value("hunter2");

  sync_pb::EncryptedData encrypted1;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted1));
  sync_pb::EncryptedData encrypted2;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted2));

  KeyParams params2 = {"localhost", "dummy", "dummy2"};
  EXPECT_TRUE(cryptographer_.AddKey(params2));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted3;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted3));
  sync_pb::EncryptedData encrypted4;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted4));

  EXPECT_EQ(encrypted1.key_name(), encrypted2.key_name());
  EXPECT_NE(encrypted1.key_name(), encrypted3.key_name());
  EXPECT_EQ(encrypted3.key_name(), encrypted4.key_name());
}

TEST_F(CryptographerTest, EncryptExportDecrypt) {
  sync_pb::EncryptedData nigori;
  sync_pb::EncryptedData encrypted;

  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("azure");
  original.set_password_value("hunter2");

  {
    Cryptographer cryptographer(&encryptor_);

    KeyParams params = {"localhost", "dummy", "dummy"};
    cryptographer.AddKey(params);
    EXPECT_TRUE(cryptographer.is_ready());

    EXPECT_TRUE(cryptographer.Encrypt(original, &encrypted));
    EXPECT_TRUE(cryptographer.GetKeys(&nigori));
  }

  {
    Cryptographer cryptographer(&encryptor_);
    EXPECT_FALSE(cryptographer.CanDecrypt(nigori));

    cryptographer.SetPendingKeys(nigori);
    EXPECT_FALSE(cryptographer.is_ready());
    EXPECT_TRUE(cryptographer.has_pending_keys());

    KeyParams params = {"localhost", "dummy", "dummy"};
    EXPECT_TRUE(cryptographer.DecryptPendingKeys(params));
    EXPECT_TRUE(cryptographer.is_ready());
    EXPECT_FALSE(cryptographer.has_pending_keys());

    sync_pb::PasswordSpecificsData decrypted;
    EXPECT_TRUE(cryptographer.Decrypt(encrypted, &decrypted));
    EXPECT_EQ(original.SerializeAsString(), decrypted.SerializeAsString());
  }
}

TEST_F(CryptographerTest, Bootstrap) {
  KeyParams params = {"localhost", "dummy", "dummy"};
  cryptographer_.AddKey(params);

  std::string token;
  EXPECT_TRUE(cryptographer_.GetBootstrapToken(&token));
  EXPECT_TRUE(base::IsStringUTF8(token));

  Cryptographer other_cryptographer(&encryptor_);
  other_cryptographer.Bootstrap(token);
  EXPECT_TRUE(other_cryptographer.is_ready());

  const char secret[] = "secret";
  sync_pb::EncryptedData encrypted;
  EXPECT_TRUE(other_cryptographer.EncryptString(secret, &encrypted));
  EXPECT_TRUE(cryptographer_.CanDecryptUsingDefaultKey(encrypted));
}

// Verifies that copied cryptographers are just as good as the original.
//
// Encrypt an item using the original cryptographer and two different sets of
// keys.  Verify that it can decrypt them.
//
// Then copy the original cryptographer and ensure it can also decrypt these
// items and encrypt them with the most recent key.
TEST_F(CryptographerTest, CopyConstructor) {
  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("luser");
  original.set_password_value("p4ssw0rd");

  // Start by testing the original cryptogprapher.
  KeyParams params1 = {"localhost", "dummy", "dummy"};
  EXPECT_TRUE(cryptographer_.AddKey(params1));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted_k1;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted_k1));

  KeyParams params2 = {"localhost", "fatuous", "fatuous"};
  EXPECT_TRUE(cryptographer_.AddKey(params2));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted_k2;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted_k2));

  sync_pb::PasswordSpecificsData decrypted_k1;
  sync_pb::PasswordSpecificsData decrypted_k2;
  EXPECT_TRUE(cryptographer_.Decrypt(encrypted_k1, &decrypted_k1));
  EXPECT_TRUE(cryptographer_.Decrypt(encrypted_k2, &decrypted_k2));

  EXPECT_EQ(original.SerializeAsString(), decrypted_k1.SerializeAsString());
  EXPECT_EQ(original.SerializeAsString(), decrypted_k2.SerializeAsString());

  // Clone the cryptographer and test that it behaves the same.
  Cryptographer cryptographer_clone(cryptographer_);

  // The clone should be able to decrypt with old and new keys.
  sync_pb::PasswordSpecificsData decrypted_k1_clone;
  sync_pb::PasswordSpecificsData decrypted_k2_clone;
  EXPECT_TRUE(cryptographer_clone.Decrypt(encrypted_k1, &decrypted_k1_clone));
  EXPECT_TRUE(cryptographer_clone.Decrypt(encrypted_k2, &decrypted_k2_clone));

  EXPECT_EQ(original.SerializeAsString(),
            decrypted_k1_clone.SerializeAsString());
  EXPECT_EQ(original.SerializeAsString(),
            decrypted_k2_clone.SerializeAsString());

  // The old cryptographer should be able to decrypt things encrypted by the
  // new.
  sync_pb::EncryptedData encrypted_c;
  EXPECT_TRUE(cryptographer_clone.Encrypt(original, &encrypted_c));

  sync_pb::PasswordSpecificsData decrypted_c;
  EXPECT_TRUE(cryptographer_.Decrypt(encrypted_c, &decrypted_c));
  EXPECT_EQ(original.SerializeAsString(), decrypted_c.SerializeAsString());

  // The cloned cryptographer should be using the latest key.
  EXPECT_EQ(encrypted_c.key_name(), encrypted_k2.key_name());
}

// Test verifies that GetBootstrapToken/Bootstrap only transfers default
// key. Additional call to GetKeys/InstallKeys is needed to transfer keybag
// to decrypt messages encrypted with old keys.
TEST_F(CryptographerTest, GetKeysThenInstall) {
  sync_pb::PasswordSpecificsData original;
  original.set_origin("http://example.com");
  original.set_username_value("luser");
  original.set_password_value("p4ssw0rd");

  // First, encrypt the same value using two different keys.
  KeyParams params1 = {"localhost", "dummy", "dummy"};
  EXPECT_TRUE(cryptographer_.AddKey(params1));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted_k1;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted_k1));

  KeyParams params2 = {"localhost", "dummy2", "dummy2"};
  EXPECT_TRUE(cryptographer_.AddKey(params2));
  EXPECT_TRUE(cryptographer_.is_ready());

  sync_pb::EncryptedData encrypted_k2;
  EXPECT_TRUE(cryptographer_.Encrypt(original, &encrypted_k2));

  // Then construct second cryptographer and bootstrap it from the first one.
  Cryptographer another_cryptographer(cryptographer_.encryptor());
  std::string bootstrap_token;
  EXPECT_TRUE(cryptographer_.GetBootstrapToken(&bootstrap_token));
  another_cryptographer.Bootstrap(bootstrap_token);

  // Before key installation, the second cryptographer should only be able
  // to decrypt using the last key.
  EXPECT_FALSE(another_cryptographer.CanDecrypt(encrypted_k1));
  EXPECT_TRUE(another_cryptographer.CanDecrypt(encrypted_k2));

  sync_pb::EncryptedData keys;
  EXPECT_TRUE(cryptographer_.GetKeys(&keys));
  ASSERT_TRUE(another_cryptographer.CanDecrypt(keys));
  another_cryptographer.InstallKeys(keys);

  // Verify that bootstrapped cryptographer decrypts succesfully using
  // all the keys after key installation.
  EXPECT_TRUE(another_cryptographer.CanDecrypt(encrypted_k1));
  EXPECT_TRUE(another_cryptographer.CanDecrypt(encrypted_k2));
}

}  // namespace syncer
