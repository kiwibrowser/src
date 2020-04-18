// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_to_device_secure_context.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/proto/securemessage.pb.h"
#include "components/cryptauth/session_keys.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

namespace {

const char kSymmetricKey[] = "symmetric key";
const char kResponderAuthMessage[] = "responder_auth_message";
const SecureContext::ProtocolVersion kProtocolVersion =
    SecureContext::PROTOCOL_VERSION_THREE_ONE;

// Callback saving |result| to |result_out|.
void SaveResult(std::string* result_out, const std::string& result) {
  *result_out = result;
}

// The responder's secure context will have the encoding / decoding keys
// inverted.
class InvertedSessionKeys : public SessionKeys {
 public:
  explicit InvertedSessionKeys(const std::string& master_symmetric_key)
      : SessionKeys(master_symmetric_key) {}

  InvertedSessionKeys() : SessionKeys() {}

  InvertedSessionKeys(const InvertedSessionKeys& other) : SessionKeys(other) {}

  std::string initiator_encode_key() const override {
    return SessionKeys::responder_encode_key();
  }
  std::string responder_encode_key() const override {
    return SessionKeys::initiator_encode_key();
  }
};

}  // namespace

class ProximityAuthDeviceToDeviceSecureContextTest : public testing::Test {
 protected:
  ProximityAuthDeviceToDeviceSecureContextTest()
      : secure_context_(std::make_unique<FakeSecureMessageDelegate>(),
                        SessionKeys(kSymmetricKey),
                        kResponderAuthMessage,
                        kProtocolVersion) {}

  DeviceToDeviceSecureContext secure_context_;
};

TEST_F(ProximityAuthDeviceToDeviceSecureContextTest, GetProperties) {
  EXPECT_EQ(kResponderAuthMessage, secure_context_.GetChannelBindingData());
  EXPECT_EQ(kProtocolVersion, secure_context_.GetProtocolVersion());
}

TEST_F(ProximityAuthDeviceToDeviceSecureContextTest, CheckEncodedHeader) {
  std::string message = "encrypt this message";
  std::string encoded_message;
  secure_context_.Encode(message, base::Bind(&SaveResult, &encoded_message));

  securemessage::SecureMessage secure_message;
  ASSERT_TRUE(secure_message.ParseFromString(encoded_message));
  securemessage::HeaderAndBody header_and_body;
  ASSERT_TRUE(
      header_and_body.ParseFromString(secure_message.header_and_body()));

  GcmMetadata gcm_metadata;
  ASSERT_TRUE(
      gcm_metadata.ParseFromString(header_and_body.header().public_metadata()));
  EXPECT_EQ(1, gcm_metadata.version());
  EXPECT_EQ(DEVICE_TO_DEVICE_MESSAGE, gcm_metadata.type());
}

TEST_F(ProximityAuthDeviceToDeviceSecureContextTest, DecodeInvalidMessage) {
  std::string encoded_message = "invalidly encoded message";
  std::string decoded_message = "not empty";
  secure_context_.Decode(encoded_message,
                         base::Bind(&SaveResult, &decoded_message));
  EXPECT_TRUE(decoded_message.empty());
}

TEST_F(ProximityAuthDeviceToDeviceSecureContextTest, EncodeAndDecode) {
  // Initialize second secure channel with the same parameters as the first.
  InvertedSessionKeys inverted_session_keys(kSymmetricKey);
  DeviceToDeviceSecureContext secure_context2(
      std::make_unique<FakeSecureMessageDelegate>(), inverted_session_keys,
      kResponderAuthMessage, kProtocolVersion);
  std::string message = "encrypt this message";

  SessionKeys session_keys(kSymmetricKey);
  EXPECT_EQ(session_keys.initiator_encode_key(),
            inverted_session_keys.responder_encode_key());
  EXPECT_EQ(session_keys.responder_encode_key(),
            inverted_session_keys.initiator_encode_key());

  // Pass some messages between the two secure contexts.
  for (int i = 0; i < 3; ++i) {
    std::string encoded_message;
    secure_context_.Encode(message, base::Bind(&SaveResult, &encoded_message));
    EXPECT_NE(message, encoded_message);

    std::string decoded_message;
    secure_context2.Decode(encoded_message,
                           base::Bind(&SaveResult, &decoded_message));
    EXPECT_EQ(message, decoded_message);
  }
}

TEST_F(ProximityAuthDeviceToDeviceSecureContextTest,
       DecodeInvalidSequenceNumber) {
  // Initialize second secure channel with the same parameters as the first.
  DeviceToDeviceSecureContext secure_context2(
      std::make_unique<FakeSecureMessageDelegate>(),
      InvertedSessionKeys(kSymmetricKey), kResponderAuthMessage,
      kProtocolVersion);

  // Send a few messages over the first secure context.
  std::string message = "encrypt this message";
  std::string encoded1;
  for (int i = 0; i < 3; ++i) {
    secure_context_.Encode(message, base::Bind(&SaveResult, &encoded1));
  }

  // Second secure channel should not decode the message with an invalid
  // sequence number.
  std::string decoded_message = "not empty";
  secure_context_.Decode(encoded1, base::Bind(&SaveResult, &decoded_message));
  EXPECT_TRUE(decoded_message.empty());
}

}  // cryptauth
