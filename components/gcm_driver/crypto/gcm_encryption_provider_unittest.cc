// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/crypto/gcm_encryption_provider.h"

#include <stddef.h>

#include <sstream>
#include <string>

#include "base/base64url.h"
#include "base/big_endian.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/test/histogram_tester.h"
#include "components/gcm_driver/common/gcm_messages.h"
#include "components/gcm_driver/crypto/gcm_decryption_result.h"
#include "components/gcm_driver/crypto/gcm_key_store.h"
#include "components/gcm_driver/crypto/gcm_message_cryptographer.h"
#include "components/gcm_driver/crypto/p256_key_util.h"
#include "crypto/random.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gcm {
namespace {

const char kExampleAppId[] = "my-app-id";
const char kExampleAuthorizedEntity[] = "my-sender-id";
const char kExampleMessage[] = "Hello, world, this is the GCM Driver!";

const char kValidEncryptionHeader[] =
    "keyid=foo;salt=MTIzNDU2Nzg5MDEyMzQ1Ng;rs=1024";
const char kInvalidEncryptionHeader[] = "keyid";

const char kValidCryptoKeyHeader[] =
    "keyid=foo;dh=BL_UGhfudEkXMUd4U4-D4nP5KHxKjQHsW6j88ybbehXM7fqi1OMFefDUEi0eJ"
    "vsKfyVBWYkQjH-lSPJKxjAyslg";
const char kValidThreeValueCryptoKeyHeader[] =
    "keyid=foo,keyid=bar,keyid=baz;dh=BL_UGhfudEkXMUd4U4-D4nP5KHxKjQHsW6j88ybbe"
    "hXM7fqi1OMFefDUEi0eJvsKfyVBWYkQjH-lSPJKxjAyslg";

const char kInvalidCryptoKeyHeader[] = "keyid";
const char kInvalidThreeValueCryptoKeyHeader[] =
    "keyid=foo,dh=BL_UGhfudEkXMUd4U4-D4nP5KHxKjQHsW6j88ybbehXM7fqi1OMFefDUEi0eJ"
    "vsKfyVBWYkQjH-lSPJKxjAyslg,keyid=baz,dh=BL_UGhfudEkXMUd4U4-D4nP5KHxKjQHsW6"
    "j88ybbehXM7fqi1OMFefDUEi0eJvsKfyVBWYkQjH-lSPJKxjAyslg";

}  // namespace

using ECPrivateKeyUniquePtr = std::unique_ptr<crypto::ECPrivateKey>;

class GCMEncryptionProviderTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());

    encryption_provider_.reset(new GCMEncryptionProvider);
    encryption_provider_->Init(scoped_temp_dir_.GetPath(),
                               message_loop_.task_runner());
  }

  void TearDown() override {
    encryption_provider_.reset();

    // |encryption_provider_| owns a ProtoDatabaseImpl whose destructor deletes
    // the underlying LevelDB database on the task runner.
    base::RunLoop().RunUntilIdle();
  }

  // To be used as a callback for GCMEncryptionProvider::GetEncryptionInfo().
  void DidGetEncryptionInfo(std::string* p256dh_out,
                            std::string* auth_secret_out,
                            const std::string& p256dh,
                            const std::string& auth_secret) {
    *p256dh_out = p256dh;
    *auth_secret_out = auth_secret;
  }

  // To be used as a callback for GCMKeyStore::{GetKeys,CreateKeys}.
  void HandleKeysCallback(ECPrivateKeyUniquePtr* key_out,
                          std::string* auth_secret_out,
                          ECPrivateKeyUniquePtr key,
                          const std::string& auth_secret) {
    *key_out = std::move(key);
    *auth_secret_out = auth_secret;
  }

 protected:
  // Decrypts the |message| and then synchronously waits until either the
  // success or failure callbacks has been invoked.
  void Decrypt(const IncomingMessage& message) {
    encryption_provider_->DecryptMessage(
        kExampleAppId, message,
        base::Bind(&GCMEncryptionProviderTest::DidDecryptMessage,
                   base::Unretained(this)));

    // The encryption keys will be read asynchronously.
    base::RunLoop().RunUntilIdle();
  }

  // Checks that the underlying key store has a key for the |kExampleAppId| +
  // authorized entity key if and only if |should_have_key| is true. Must wrap
  // with ASSERT/EXPECT_NO_FATAL_FAILURE.
  void CheckHasKey(const std::string& authorized_entity, bool should_have_key) {
    ECPrivateKeyUniquePtr key;
    std::string auth_secret;
    encryption_provider()->key_store_->GetKeys(
        kExampleAppId, authorized_entity,
        false /* fallback_to_empty_authorized_entity */,
        base::BindOnce(&GCMEncryptionProviderTest::HandleKeysCallback,
                       base::Unretained(this), &key, &auth_secret));

    base::RunLoop().RunUntilIdle();

    if (should_have_key) {
      ASSERT_TRUE(key);
      std::string private_key, public_key;
      ASSERT_TRUE(GetRawPrivateKey(*key, &private_key));
      ASSERT_TRUE(GetRawPublicKey(*key, &public_key));
      ASSERT_GT(public_key.size(), 0u);
      ASSERT_GT(private_key.size(), 0u);
      ASSERT_GT(auth_secret.size(), 0u);
    } else {
      ASSERT_FALSE(key);
      ASSERT_EQ(0u, auth_secret.size());
    }
  }

  // Returns the result of the previous decryption operation.
  GCMDecryptionResult decryption_result() { return decryption_result_; }

  // Returns the message resulting from the previous decryption operation.
  const IncomingMessage& decrypted_message() { return decrypted_message_; }

  GCMEncryptionProvider* encryption_provider() {
    return encryption_provider_.get();
  }

  // Performs a full round-trip test of the encryption feature. Must wrap this
  // in ASSERT_NO_FATAL_FAILURE.
  void TestEncryptionRoundTrip(const std::string& app_id,
                               const std::string& authorized_entity,
                               GCMMessageCryptographer::Version version);

 private:
  void DidDecryptMessage(GCMDecryptionResult result,
                         const IncomingMessage& message) {
    decryption_result_ = result;
    decrypted_message_ = message;
  }

  base::MessageLoop message_loop_;
  base::ScopedTempDir scoped_temp_dir_;
  base::HistogramTester histogram_tester_;

  std::unique_ptr<GCMEncryptionProvider> encryption_provider_;

  GCMDecryptionResult decryption_result_ = GCMDecryptionResult::UNENCRYPTED;

  IncomingMessage decrypted_message_;
};

TEST_F(GCMEncryptionProviderTest, IsEncryptedMessage) {
  // Both the Encryption and Encryption-Key headers must be present, and the raw
  // data must be non-empty for a message to be considered encrypted.

  IncomingMessage empty_message;
  EXPECT_FALSE(encryption_provider()->IsEncryptedMessage(empty_message));

  IncomingMessage single_header_message;
  single_header_message.data["encryption"] = "";
  EXPECT_FALSE(encryption_provider()->IsEncryptedMessage(
                   single_header_message));

  IncomingMessage double_header_message;
  double_header_message.data["encryption"] = "";
  double_header_message.data["crypto-key"] = "";
  EXPECT_FALSE(encryption_provider()->IsEncryptedMessage(
                   double_header_message));

  IncomingMessage double_header_with_data_message;
  double_header_with_data_message.data["encryption"] = "";
  double_header_with_data_message.data["crypto-key"] = "";
  double_header_with_data_message.raw_data = "foo";
  EXPECT_TRUE(encryption_provider()->IsEncryptedMessage(
                  double_header_with_data_message));

  IncomingMessage draft08_message;
  draft08_message.data["content-encoding"] = "aes128gcm";
  draft08_message.raw_data = "foo";
  EXPECT_TRUE(encryption_provider()->IsEncryptedMessage(draft08_message));
}

TEST_F(GCMEncryptionProviderTest, VerifiesEncryptionHeaderParsing) {
  // The Encryption header must be parsable and contain valid values.
  // Note that this is more extensively tested in EncryptionHeaderParsersTest.

  IncomingMessage invalid_message;
  invalid_message.data["encryption"] = kInvalidEncryptionHeader;
  invalid_message.data["crypto-key"] = kValidCryptoKeyHeader;
  invalid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(invalid_message));
  EXPECT_EQ(GCMDecryptionResult::INVALID_ENCRYPTION_HEADER,
            decryption_result());

  IncomingMessage valid_message;
  valid_message.data["encryption"] = kValidEncryptionHeader;
  valid_message.data["crypto-key"] = kInvalidCryptoKeyHeader;
  valid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(valid_message));
  EXPECT_NE(GCMDecryptionResult::INVALID_ENCRYPTION_HEADER,
            decryption_result());
}

TEST_F(GCMEncryptionProviderTest, VerifiesCryptoKeyHeaderParsing) {
  // The Crypto-Key header must be parsable and contain valid values.
  // Note that this is more extensively tested in EncryptionHeaderParsersTest.

  IncomingMessage invalid_message;
  invalid_message.data["encryption"] = kValidEncryptionHeader;
  invalid_message.data["crypto-key"] = kInvalidCryptoKeyHeader;
  invalid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(invalid_message));
  EXPECT_EQ(GCMDecryptionResult::INVALID_CRYPTO_KEY_HEADER,
            decryption_result());

  IncomingMessage valid_message;
  valid_message.data["encryption"] = kValidEncryptionHeader;
  valid_message.data["crypto-key"] = kValidCryptoKeyHeader;
  valid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(valid_message));
  EXPECT_NE(GCMDecryptionResult::INVALID_CRYPTO_KEY_HEADER,
            decryption_result());
}

TEST_F(GCMEncryptionProviderTest, VerifiesCryptoKeyHeaderParsingThirdValue) {
  // The Crypto-Key header must be parsable and contain valid values, in which
  // values will be ignored unless they contain a "dh" property.

  IncomingMessage valid_message;
  valid_message.data["encryption"] = kValidEncryptionHeader;
  valid_message.data["crypto-key"] = kValidThreeValueCryptoKeyHeader;
  valid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(valid_message));
  EXPECT_NE(GCMDecryptionResult::INVALID_CRYPTO_KEY_HEADER,
            decryption_result());
}

TEST_F(GCMEncryptionProviderTest, VerifiesCryptoKeyHeaderSingleDhEntry) {
  // The Crypto-Key header must include at most one value that contains the
  // "dh" property. Having more than once occurrence is forbidden.

  IncomingMessage valid_message;
  valid_message.data["encryption"] = kValidEncryptionHeader;
  valid_message.data["crypto-key"] = kInvalidThreeValueCryptoKeyHeader;
  valid_message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(valid_message));
  EXPECT_EQ(GCMDecryptionResult::INVALID_CRYPTO_KEY_HEADER,
            decryption_result());
}

TEST_F(GCMEncryptionProviderTest, VerifiesExistingKeys) {
  // When both headers are valid, the encryption keys still must be known to
  // the GCM key store before the message can be decrypted.

  IncomingMessage message;
  message.data["encryption"] = kValidEncryptionHeader;
  message.data["crypto-key"] = kValidCryptoKeyHeader;
  message.raw_data = "foo";

  ASSERT_NO_FATAL_FAILURE(Decrypt(message));
  EXPECT_EQ(GCMDecryptionResult::NO_KEYS, decryption_result());

  std::string public_key, auth_secret;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, "" /* empty authorized entity for non-InstanceID */,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &public_key, &auth_secret));

  // Getting (or creating) the public key will be done asynchronously.
  base::RunLoop().RunUntilIdle();

  ASSERT_GT(public_key.size(), 0u);
  ASSERT_GT(auth_secret.size(), 0u);

  ASSERT_NO_FATAL_FAILURE(Decrypt(message));
  EXPECT_NE(GCMDecryptionResult::NO_KEYS, decryption_result());
}

TEST_F(GCMEncryptionProviderTest, VerifiesKeyRemovalGCMRegistration) {
  // Removing encryption info for an InstanceID token shouldn't affect a
  // non-InstanceID GCM registration.

  // Non-InstanceID callers pass an empty string for authorized_entity.
  std::string authorized_entity_gcm;
  std::string authorized_entity_1 = kExampleAuthorizedEntity + std::string("1");
  std::string authorized_entity_2 = kExampleAuthorizedEntity + std::string("2");

  // Should create encryption info.
  std::string public_key, auth_secret;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_gcm,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &public_key, &auth_secret));

  base::RunLoop().RunUntilIdle();

  // Should get encryption info created above.
  std::string read_public_key, read_auth_secret;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_gcm,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &read_public_key, &read_auth_secret));

  base::RunLoop().RunUntilIdle();

  EXPECT_GT(public_key.size(), 0u);
  EXPECT_GT(auth_secret.size(), 0u);
  EXPECT_EQ(public_key, read_public_key);
  EXPECT_EQ(auth_secret, read_auth_secret);

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_gcm, true));

  encryption_provider()->RemoveEncryptionInfo(
      kExampleAppId, authorized_entity_1, base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_gcm, true));

  encryption_provider()->RemoveEncryptionInfo(kExampleAppId, "*",
                                              base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_gcm, true));

  encryption_provider()->RemoveEncryptionInfo(
      kExampleAppId, authorized_entity_gcm, base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_gcm, false));
}

TEST_F(GCMEncryptionProviderTest, VerifiesKeyRemovalInstanceIDToken) {
  // Removing encryption info for a non-InstanceID GCM registration shouldn't
  // affect an InstanceID token.

  // Non-InstanceID callers pass an empty string for authorized_entity.
  std::string authorized_entity_gcm;
  std::string authorized_entity_1 = kExampleAuthorizedEntity + std::string("1");
  std::string authorized_entity_2 = kExampleAuthorizedEntity + std::string("2");

  std::string public_key_1, auth_secret_1;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_1,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &public_key_1, &auth_secret_1));

  base::RunLoop().RunUntilIdle();

  EXPECT_GT(public_key_1.size(), 0u);
  EXPECT_GT(auth_secret_1.size(), 0u);

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, true));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, false));

  std::string public_key_2, auth_secret_2;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_2,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &public_key_2, &auth_secret_2));

  base::RunLoop().RunUntilIdle();

  EXPECT_GT(public_key_2.size(), 0u);
  EXPECT_GT(auth_secret_2.size(), 0u);
  EXPECT_NE(public_key_1, public_key_2);
  EXPECT_NE(auth_secret_1, auth_secret_2);

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, true));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, true));

  std::string read_public_key_1, read_auth_secret_1;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_1,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &read_public_key_1,
                 &read_auth_secret_1));

  base::RunLoop().RunUntilIdle();

  // Should have returned existing info for authorized_entity_1.
  EXPECT_EQ(public_key_1, read_public_key_1);
  EXPECT_EQ(auth_secret_1, read_auth_secret_1);

  encryption_provider()->RemoveEncryptionInfo(
      kExampleAppId, authorized_entity_gcm, base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, true));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, true));

  encryption_provider()->RemoveEncryptionInfo(
      kExampleAppId, authorized_entity_1, base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, false));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, true));

  std::string public_key_1_refreshed, auth_secret_1_refreshed;
  encryption_provider()->GetEncryptionInfo(
      kExampleAppId, authorized_entity_1,
      base::Bind(&GCMEncryptionProviderTest::DidGetEncryptionInfo,
                 base::Unretained(this), &public_key_1_refreshed,
                 &auth_secret_1_refreshed));

  base::RunLoop().RunUntilIdle();

  // Since the info was removed, GetEncryptionInfo should have created new info.
  EXPECT_GT(public_key_1_refreshed.size(), 0u);
  EXPECT_GT(auth_secret_1_refreshed.size(), 0u);
  EXPECT_NE(public_key_1, public_key_1_refreshed);
  EXPECT_NE(auth_secret_1, auth_secret_1_refreshed);
  EXPECT_NE(public_key_2, public_key_1_refreshed);
  EXPECT_NE(auth_secret_2, auth_secret_1_refreshed);

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, true));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, true));

  encryption_provider()->RemoveEncryptionInfo(kExampleAppId, "*",
                                              base::DoNothing());

  base::RunLoop().RunUntilIdle();

  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_1, false));
  ASSERT_NO_FATAL_FAILURE(CheckHasKey(authorized_entity_2, false));
}

void GCMEncryptionProviderTest::TestEncryptionRoundTrip(
    const std::string& app_id,
    const std::string& authorized_entity,
    GCMMessageCryptographer::Version version) {
  // Performs a full round-trip of the encryption feature, including getting a
  // public/private key-key and performing the cryptographic operations. This
  // is more of an integration test than a unit test.

  ECPrivateKeyUniquePtr key, server_key;
  std::string auth_secret, server_authentication;

  // Retrieve the public/private key-key immediately from the key store, given
  // that the GCMEncryptionProvider will only share the public key with users.
  // Also create a second key, which will act as the server's keys.
  encryption_provider()->key_store_->CreateKeys(
      app_id, authorized_entity,
      base::BindOnce(&GCMEncryptionProviderTest::HandleKeysCallback,
                     base::Unretained(this), &key, &auth_secret));

  encryption_provider()->key_store_->CreateKeys(
      "server-" + app_id, authorized_entity,
      base::BindOnce(&GCMEncryptionProviderTest::HandleKeysCallback,
                     base::Unretained(this), &server_key,
                     &server_authentication));

  // Creating the public keys will be done asynchronously.
  base::RunLoop().RunUntilIdle();

  std::string public_key, server_public_key;
  ASSERT_TRUE(GetRawPublicKey(*key, &public_key));
  ASSERT_TRUE(GetRawPublicKey(*server_key, &server_public_key));
  ASSERT_GT(public_key.size(), 0u);
  ASSERT_GT(server_public_key.size(), 0u);

  std::string private_key, server_private_key;
  ASSERT_TRUE(GetRawPublicKey(*key, &private_key));
  ASSERT_TRUE(GetRawPublicKey(*server_key, &server_private_key));
  ASSERT_GT(private_key.size(), 0u);
  ASSERT_GT(server_private_key.size(), 0u);

  std::string salt;

  // Creates a cryptographically secure salt of |salt_size| octets in size, and
  // calculate the shared secret for the message.
  crypto::RandBytes(base::WriteInto(&salt, 16 + 1), 16);

  std::string shared_secret;
  ASSERT_TRUE(ComputeSharedP256Secret(*key, server_public_key, &shared_secret));

  IncomingMessage message;
  size_t record_size;

  message.sender_id = kExampleAuthorizedEntity;

  // Encrypts the |kExampleMessage| using the generated shared key and the
  // random |salt|, storing the result in |record_size| and the message.
  GCMMessageCryptographer cryptographer(version);

  std::string ciphertext;
  ASSERT_TRUE(cryptographer.Encrypt(
      public_key, server_public_key, shared_secret, auth_secret, salt,
      kExampleMessage, &record_size, &ciphertext));

  switch (version) {
    case GCMMessageCryptographer::Version::DRAFT_03: {
      std::string encoded_salt, encoded_key;

      // Compile the incoming GCM message, including the required headers.
      base::Base64UrlEncode(salt, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                            &encoded_salt);
      base::Base64UrlEncode(server_public_key,
                            base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                            &encoded_key);

      std::stringstream encryption_header;
      encryption_header << "rs=" << base::NumberToString(record_size) << ";";
      encryption_header << "salt=" << encoded_salt;

      message.data["encryption"] = encryption_header.str();
      message.data["crypto-key"] = "dh=" + encoded_key;
      message.raw_data.swap(ciphertext);
      break;
    }
    case GCMMessageCryptographer::Version::DRAFT_08: {
      uint32_t rs = record_size;
      uint8_t key_length = server_public_key.size();

      std::vector<char> payload(salt.size() + sizeof(rs) + sizeof(key_length) +
                                server_public_key.size() + ciphertext.size());

      char* current = &payload.front();

      memcpy(current, salt.data(), salt.size());
      current += salt.size();

      base::WriteBigEndian(current, rs);
      current += sizeof(rs);

      base::WriteBigEndian(current, key_length);
      current += sizeof(key_length);

      memcpy(current, server_public_key.data(), server_public_key.size());
      current += server_public_key.size();

      memcpy(current, ciphertext.data(), ciphertext.size());

      message.data["content-encoding"] = "aes128gcm";
      message.raw_data.assign(payload.begin(), payload.end());
      break;
    }
  }

  ASSERT_TRUE(encryption_provider()->IsEncryptedMessage(message));

  // Decrypt the message, and expect everything to go wonderfully well.
  ASSERT_NO_FATAL_FAILURE(Decrypt(message));
  ASSERT_EQ(version == GCMMessageCryptographer::Version::DRAFT_03
                ? GCMDecryptionResult::DECRYPTED_DRAFT_03
                : GCMDecryptionResult::DECRYPTED_DRAFT_08,
            decryption_result());

  EXPECT_TRUE(decrypted_message().decrypted);
  EXPECT_EQ(kExampleMessage, decrypted_message().raw_data);
}

TEST_F(GCMEncryptionProviderTest, EncryptionRoundTripGCMRegistration) {
  // GCMEncryptionProvider::DecryptMessage should succeed when the message was
  // sent to a non-InstanceID GCM registration (empty authorized_entity).
  ASSERT_NO_FATAL_FAILURE(TestEncryptionRoundTrip(
      kExampleAppId, "" /* empty authorized entity for non-InstanceID */,
      GCMMessageCryptographer::Version::DRAFT_03));
}

TEST_F(GCMEncryptionProviderTest, EncryptionRoundTripInstanceIDToken) {
  // GCMEncryptionProvider::DecryptMessage should succeed when the message was
  // sent to an InstanceID token (non-empty authorized_entity).
  ASSERT_NO_FATAL_FAILURE(
      TestEncryptionRoundTrip(kExampleAppId, kExampleAuthorizedEntity,
                              GCMMessageCryptographer::Version::DRAFT_03));
}

TEST_F(GCMEncryptionProviderTest, EncryptionRoundTripDraft08) {
  // GCMEncryptionProvider::DecryptMessage should succeed when the message was
  // encrypted following raft-ietf-webpush-encryption-08.
  ASSERT_NO_FATAL_FAILURE(
      TestEncryptionRoundTrip(kExampleAppId, kExampleAuthorizedEntity,
                              GCMMessageCryptographer::Version::DRAFT_08));
}

}  // namespace gcm
