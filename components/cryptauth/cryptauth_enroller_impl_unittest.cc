// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_enroller_impl.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/cryptauth_enrollment_utils.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

namespace cryptauth {

namespace {

const char kAccessTokenUsed[] = "access token used by CryptAuthClient";

const char kClientSessionPublicKey[] = "throw away after one use";
const char kServerSessionPublicKey[] = "disposables are not eco-friendly";

InvocationReason kInvocationReason =
    INVOCATION_REASON_MANUAL;
const int kGCMMetadataVersion = 1;
const char kSupportedEnrollmentTypeGcmV1[] = "gcmV1";
const char kResponseStatusOk[] = "ok";
const char kResponseStatusNotOk[] = "Your key was too bland.";
const char kEnrollmentSessionId[] = "0123456789876543210";
const char kFinishEnrollmentError[] = "A hungry router ate all your packets.";

const char kDeviceId[] = "2015 AD";
const DeviceType kDeviceType = CHROME;
const char kDeviceOsVersion[] = "41.0.0";

// Creates and returns the GcmDeviceInfo message to be uploaded.
GcmDeviceInfo GetDeviceInfo() {
  GcmDeviceInfo device_info;
  device_info.set_long_device_id(kDeviceId);
  device_info.set_device_type(kDeviceType);
  device_info.set_device_os_version(kDeviceOsVersion);
  return device_info;
}

// Creates and returns the SetupEnrollmentResponse message to be returned to the
// enroller with the session_. If |success| is false, then a bad response will
// be returned.
SetupEnrollmentResponse GetSetupEnrollmentResponse(bool success) {
  SetupEnrollmentResponse response;
  if (!success) {
    response.set_status(kResponseStatusNotOk);
    return response;
  }

  response.set_status(kResponseStatusOk);
  SetupEnrollmentInfo* info = response.add_infos();
  info->set_type(kSupportedEnrollmentTypeGcmV1);
  info->set_enrollment_session_id(kEnrollmentSessionId);
  info->set_server_ephemeral_key(kServerSessionPublicKey);
  return response;
}

// Creates and returns the FinishEnrollmentResponse message to be returned to
// the enroller with the session_. If |success| is false, then a bad response
// will be returned.
FinishEnrollmentResponse GetFinishEnrollmentResponse(bool success) {
  FinishEnrollmentResponse response;
  if (success) {
    response.set_status(kResponseStatusOk);
  } else {
    response.set_status(kResponseStatusNotOk);
    response.set_error_message(kFinishEnrollmentError);
  }
  return response;
}

// Callback that saves the key returned by SecureMessageDelegate::DeriveKey().
void SaveDerivedKey(std::string* value_out, const std::string& value) {
  *value_out = value;
}

// Callback that saves the results returned by
// SecureMessageDelegate::UnwrapSecureMessage().
void SaveUnwrapResults(bool* verified_out,
                       std::string* payload_out,
                       securemessage::Header* header_out,
                       bool verified,
                       const std::string& payload,
                       const securemessage::Header& header) {
  *verified_out = verified;
  *payload_out = payload;
  *header_out = header;
}

}  // namespace

class CryptAuthEnrollerTest
    : public testing::Test,
      public MockCryptAuthClientFactory::Observer {
 public:
  CryptAuthEnrollerTest()
      : client_factory_(std::make_unique<MockCryptAuthClientFactory>(
            MockCryptAuthClientFactory::MockType::MAKE_NICE_MOCKS)),
        secure_message_delegate_(new FakeSecureMessageDelegate()),
        enroller_(client_factory_.get(),
                  base::WrapUnique(secure_message_delegate_)) {
    client_factory_->AddObserver(this);

    // This call is actually synchronous.
    secure_message_delegate_->GenerateKeyPair(
        base::Bind(&CryptAuthEnrollerTest::OnKeyPairGenerated,
                   base::Unretained(this)));
  }

  // Starts the enroller.
  void StartEnroller(const GcmDeviceInfo& device_info) {
    secure_message_delegate_->set_next_public_key(kClientSessionPublicKey);
    enroller_result_.reset();
    enroller_.Enroll(
        user_public_key_, user_private_key_, device_info, kInvocationReason,
        base::Bind(&CryptAuthEnrollerTest::OnEnrollerCompleted,
                   base::Unretained(this)));
  }

  // Verifies that |serialized_message| is a valid SecureMessage sent with the
  // FinishEnrollment API call.
  void ValidateEnrollmentMessage(const std::string& serialized_message) {
    // Derive the session symmetric key.
    std::string server_session_private_key =
        secure_message_delegate_->GetPrivateKeyForPublicKey(
            kServerSessionPublicKey);
    std::string symmetric_key;
    secure_message_delegate_->DeriveKey(
        server_session_private_key, kClientSessionPublicKey,
        base::Bind(&SaveDerivedKey, &symmetric_key));

    std::string inner_message;
    std::string inner_payload;
    {
      // Unwrap the outer message.
      bool verified;
      securemessage::Header header;
      SecureMessageDelegate::UnwrapOptions unwrap_options;
      unwrap_options.encryption_scheme = securemessage::AES_256_CBC;
      unwrap_options.signature_scheme = securemessage::HMAC_SHA256;
      secure_message_delegate_->UnwrapSecureMessage(
          serialized_message, symmetric_key, unwrap_options,
          base::Bind(&SaveUnwrapResults, &verified, &inner_message, &header));
      EXPECT_TRUE(verified);

      GcmMetadata metadata;
      ASSERT_TRUE(metadata.ParseFromString(header.public_metadata()));
      EXPECT_EQ(kGCMMetadataVersion, metadata.version());
      EXPECT_EQ(MessageType::ENROLLMENT, metadata.type());
    }

    {
      // Unwrap inner message.
      bool verified;
      securemessage::Header header;
      SecureMessageDelegate::UnwrapOptions unwrap_options;
      unwrap_options.encryption_scheme = securemessage::NONE;
      unwrap_options.signature_scheme = securemessage::ECDSA_P256_SHA256;
      secure_message_delegate_->UnwrapSecureMessage(
          inner_message, user_public_key_, unwrap_options,
          base::Bind(&SaveUnwrapResults, &verified, &inner_payload, &header));
      EXPECT_TRUE(verified);
      EXPECT_EQ(user_public_key_, header.verification_key_id());
    }

    // Check that the decrypted GcmDeviceInfo is correct.
    GcmDeviceInfo device_info;
    ASSERT_TRUE(device_info.ParseFromString(inner_payload));
    EXPECT_EQ(kDeviceId, device_info.long_device_id());
    EXPECT_EQ(kDeviceType, device_info.device_type());
    EXPECT_EQ(kDeviceOsVersion, device_info.device_os_version());
    EXPECT_EQ(user_public_key_, device_info.user_public_key());
    EXPECT_EQ(user_public_key_, device_info.key_handle());
    EXPECT_EQ(kEnrollmentSessionId, device_info.enrollment_session_id());
  }

 protected:
  // MockCryptAuthClientFactory::Observer:
  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, SetupEnrollment(_, _, _))
        .WillByDefault(Invoke(
            this, &CryptAuthEnrollerTest::OnSetupEnrollment));

    ON_CALL(*client, FinishEnrollment(_, _, _))
        .WillByDefault(Invoke(
            this, &CryptAuthEnrollerTest::OnFinishEnrollment));

    ON_CALL(*client, GetAccessTokenUsed())
        .WillByDefault(Return(kAccessTokenUsed));
  }

  void OnKeyPairGenerated(const std::string& public_key,
                          const std::string& private_key) {
    user_public_key_ = public_key;
    user_private_key_ = private_key;
  }

  void OnEnrollerCompleted(bool success) {
    EXPECT_FALSE(enroller_result_.get());
    enroller_result_.reset(new bool(success));
  }

  void OnSetupEnrollment(
      const SetupEnrollmentRequest& request,
      const CryptAuthClient::SetupEnrollmentCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    // Check that SetupEnrollment is called before FinishEnrollment.
    EXPECT_FALSE(setup_request_.get());
    EXPECT_FALSE(finish_request_.get());
    EXPECT_TRUE(setup_callback_.is_null());
    EXPECT_TRUE(error_callback_.is_null());

    setup_request_.reset(new SetupEnrollmentRequest(request));
    setup_callback_ = callback;
    error_callback_ = error_callback;
  }

  void OnFinishEnrollment(
      const FinishEnrollmentRequest& request,
      const CryptAuthClient::FinishEnrollmentCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    // Check that FinishEnrollment is called after SetupEnrollment.
    EXPECT_TRUE(setup_request_.get());
    EXPECT_FALSE(finish_request_.get());
    EXPECT_TRUE(finish_callback_.is_null());

    finish_request_.reset(new FinishEnrollmentRequest(request));
    finish_callback_ = callback;
    error_callback_ = error_callback;
  }

  // The persistent user key-pair.
  std::string user_public_key_;
  std::string user_private_key_;

  // Owned by |enroller_|.
  std::unique_ptr<MockCryptAuthClientFactory> client_factory_;
  // Owned by |enroller_|.
  FakeSecureMessageDelegate* secure_message_delegate_;
  // The CryptAuthEnroller under test.
  CryptAuthEnrollerImpl enroller_;

  // Stores the result of running |enroller_|.
  std::unique_ptr<bool> enroller_result_;

  // Stored callbacks and requests for SetupEnrollment and FinishEnrollment.
  std::unique_ptr<SetupEnrollmentRequest> setup_request_;
  std::unique_ptr<FinishEnrollmentRequest> finish_request_;
  CryptAuthClient::SetupEnrollmentCallback setup_callback_;
  CryptAuthClient::FinishEnrollmentCallback finish_callback_;
  CryptAuthClient::ErrorCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthEnrollerTest);
};

TEST_F(CryptAuthEnrollerTest, EnrollmentSucceeds) {
  StartEnroller(GetDeviceInfo());

  // Handle SetupEnrollment request.
  EXPECT_TRUE(setup_request_.get());
  EXPECT_EQ(kInvocationReason, setup_request_->invocation_reason());
  ASSERT_EQ(1, setup_request_->types_size());
  EXPECT_EQ(kSupportedEnrollmentTypeGcmV1, setup_request_->types(0));
  ASSERT_FALSE(setup_callback_.is_null());
  setup_callback_.Run(GetSetupEnrollmentResponse(true));

  // Handle FinishEnrollment request.
  EXPECT_TRUE(finish_request_.get());
  EXPECT_EQ(kEnrollmentSessionId, finish_request_->enrollment_session_id());
  EXPECT_EQ(kClientSessionPublicKey, finish_request_->device_ephemeral_key());
  ValidateEnrollmentMessage(finish_request_->enrollment_message());
  EXPECT_EQ(kInvocationReason, finish_request_->invocation_reason());

  ASSERT_FALSE(finish_callback_.is_null());
  finish_callback_.Run(GetFinishEnrollmentResponse(true));

  ASSERT_TRUE(enroller_result_.get());
  EXPECT_TRUE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, SetupEnrollmentApiCallError) {
  StartEnroller(GetDeviceInfo());

  EXPECT_TRUE(setup_request_.get());
  ASSERT_FALSE(error_callback_.is_null());
  error_callback_.Run("Setup enrollment failed network");

  EXPECT_TRUE(finish_callback_.is_null());
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, SetupEnrollmentBadStatus) {
  StartEnroller(GetDeviceInfo());

  EXPECT_TRUE(setup_request_.get());
  setup_callback_.Run(GetSetupEnrollmentResponse(false));

  EXPECT_TRUE(finish_callback_.is_null());
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, SetupEnrollmentNoInfosReturned) {
  StartEnroller(GetDeviceInfo());
  EXPECT_TRUE(setup_request_.get());
  SetupEnrollmentResponse response;
  response.set_status(kResponseStatusOk);
  setup_callback_.Run(response);

  EXPECT_TRUE(finish_callback_.is_null());
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, FinishEnrollmentApiCallError) {
  StartEnroller(GetDeviceInfo());
  setup_callback_.Run(GetSetupEnrollmentResponse(true));
  ASSERT_FALSE(error_callback_.is_null());
  error_callback_.Run("finish enrollment oauth error");
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, FinishEnrollmentBadStatus) {
  StartEnroller(GetDeviceInfo());
  setup_callback_.Run(GetSetupEnrollmentResponse(true));
  ASSERT_FALSE(finish_callback_.is_null());
  finish_callback_.Run(GetFinishEnrollmentResponse(false));
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, ReuseEnroller) {
  StartEnroller(GetDeviceInfo());
  setup_callback_.Run(GetSetupEnrollmentResponse(true));
  finish_callback_.Run(GetFinishEnrollmentResponse(true));
  EXPECT_TRUE(*enroller_result_);

  StartEnroller(GetDeviceInfo());
  EXPECT_FALSE(*enroller_result_);
}

TEST_F(CryptAuthEnrollerTest, IncompleteDeviceInfo) {
  StartEnroller(GcmDeviceInfo());
  ASSERT_TRUE(enroller_result_.get());
  EXPECT_FALSE(*enroller_result_);
}

}  // namespace cryptauth
