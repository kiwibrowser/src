// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_reader.h"
#include "components/cbor/cbor_values.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/device_response_converter.h"
#include "device/fido/ec_public_key.h"
#include "device/fido/fido_attestation_statement.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

// The attested credential data, excluding the public key bytes. Append
// with kTestECPublicKeyCOSE to get the complete attestation data.
constexpr uint8_t kTestAttestedCredentialDataPrefix[] = {
    // 16-byte aaguid
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    // 2-byte length
    0x00, 0x40,
    // 64-byte key handle
    0x3E, 0xBD, 0x89, 0xBF, 0x77, 0xEC, 0x50, 0x97, 0x55, 0xEE, 0x9C, 0x26,
    0x35, 0xEF, 0xAA, 0xAC, 0x7B, 0x2B, 0x9C, 0x5C, 0xEF, 0x17, 0x36, 0xC3,
    0x71, 0x7D, 0xA4, 0x85, 0x34, 0xC8, 0xC6, 0xB6, 0x54, 0xD7, 0xFF, 0x94,
    0x5F, 0x50, 0xB5, 0xCC, 0x4E, 0x78, 0x05, 0x5B, 0xDD, 0x39, 0x6B, 0x64,
    0xF7, 0x8D, 0xA2, 0xC5, 0xF9, 0x62, 0x00, 0xCC, 0xD4, 0x15, 0xCD, 0x08,
    0xFE, 0x42, 0x00, 0x38,
};

// The authenticator data, excluding the attested credential data bytes. Append
// with attested credential data to get the complete authenticator data.
constexpr uint8_t kTestAuthenticatorDataPrefix[] = {
    // sha256 hash of rp id.
    0x11, 0x94, 0x22, 0x8D, 0xA8, 0xFD, 0xBD, 0xEE, 0xFD, 0x26, 0x1B, 0xD7,
    0xB6, 0x59, 0x5C, 0xFD, 0x70, 0xA5, 0x0D, 0x70, 0xC6, 0x40, 0x7B, 0xCF,
    0x01, 0x3D, 0xE9, 0x6D, 0x4E, 0xFB, 0x17, 0xDE,
    // flags (TUP and AT bits set)
    0x41,
    // counter
    0x00, 0x00, 0x00, 0x00};

// Components of the CBOR needed to form an authenticator object.
// Combined diagnostic notation:
// {"fmt": "fido-u2f", "attStmt": {"sig": h'30...}, "authData": h'D4C9D9...'}
constexpr uint8_t kFormatFidoU2fCBOR[] = {
    // map(3)
    0xA3,
    // text(3)
    0x63,
    // "fmt"
    0x66, 0x6D, 0x74,
    // text(8)
    0x68,
    // "fido-u2f"
    0x66, 0x69, 0x64, 0x6F, 0x2D, 0x75, 0x32, 0x66};

constexpr uint8_t kAttStmtCBOR[] = {
    // text(7)
    0x67,
    // "attStmt"
    0x61, 0x74, 0x74, 0x53, 0x74, 0x6D, 0x74};

constexpr uint8_t kAuthDataCBOR[] = {
    // text(8)
    0x68,
    // "authData"
    0x61, 0x75, 0x74, 0x68, 0x44, 0x61, 0x74, 0x61,
    // bytes(196). i.e., the authenticator_data byte array corresponding to
    // kTestAuthenticatorDataPrefix|, |kTestAttestedCredentialDataPrefix|,
    // and test_data::kTestECPublicKeyCOSE.
    0x58, 0xC4};

std::vector<uint8_t> GetTestAttestedCredentialDataBytes() {
  // Combine kTestAttestedCredentialDataPrefix and kTestECPublicKeyCOSE.
  auto test_attested_data =
      fido_parsing_utils::Materialize(kTestAttestedCredentialDataPrefix);
  fido_parsing_utils::Append(&test_attested_data,
                             test_data::kTestECPublicKeyCOSE);
  return test_attested_data;
}

std::vector<uint8_t> GetTestAuthenticatorDataBytes() {
  // Build the test authenticator data.
  auto test_authenticator_data =
      fido_parsing_utils::Materialize(kTestAuthenticatorDataPrefix);
  auto test_attested_data = GetTestAttestedCredentialDataBytes();
  fido_parsing_utils::Append(&test_authenticator_data, test_attested_data);
  return test_authenticator_data;
}

std::vector<uint8_t> GetTestAttestationObjectBytes() {
  auto test_authenticator_object =
      fido_parsing_utils::Materialize(kFormatFidoU2fCBOR);
  fido_parsing_utils::Append(&test_authenticator_object, kAttStmtCBOR);
  fido_parsing_utils::Append(&test_authenticator_object,
                             test_data::kU2fAttestationStatementCBOR);
  fido_parsing_utils::Append(&test_authenticator_object, kAuthDataCBOR);
  auto test_authenticator_data = GetTestAuthenticatorDataBytes();
  fido_parsing_utils::Append(&test_authenticator_object,
                             test_authenticator_data);
  return test_authenticator_object;
}

std::vector<uint8_t> GetTestSignResponse() {
  return fido_parsing_utils::Materialize(test_data::kTestU2fSignResponse);
}

std::vector<uint8_t> GetTestSignatureCounter() {
  return fido_parsing_utils::Materialize(test_data::kTestSignatureCounter);
}

// Get a subset of the response for testing error handling.
std::vector<uint8_t> GetTestCorruptedSignResponse(size_t length) {
  DCHECK_LE(length, arraysize(test_data::kTestU2fSignResponse));
  return fido_parsing_utils::Materialize(fido_parsing_utils::ExtractSpan(
      test_data::kTestU2fSignResponse, 0, length));
}

// Return a key handle used for GetAssertion request.
std::vector<uint8_t> GetTestCredentialRawIdBytes() {
  return fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle);
}

}  // namespace

// Leveraging example 4 of section 6.1 of the spec https://fidoalliance.org
// /specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-
// 20170927.html
TEST(CTAPResponseTest, TestReadMakeCredentialResponse) {
  auto make_credential_response =
      ReadCTAPMakeCredentialResponse(test_data::kDeviceMakeCredentialResponse);
  ASSERT_TRUE(make_credential_response);
  auto cbor_attestation_object = cbor::CBORReader::Read(
      make_credential_response->GetCBOREncodedAttestationObject());
  ASSERT_TRUE(cbor_attestation_object);
  ASSERT_TRUE(cbor_attestation_object->is_map());

  const auto& attestation_object_map = cbor_attestation_object->GetMap();
  auto it = attestation_object_map.find(cbor::CBORValue(kFormatKey));
  ASSERT_TRUE(it != attestation_object_map.end());
  ASSERT_TRUE(it->second.is_string());
  EXPECT_EQ(it->second.GetString(), "packed");

  it = attestation_object_map.find(cbor::CBORValue(kAuthDataKey));
  ASSERT_TRUE(it != attestation_object_map.end());
  ASSERT_TRUE(it->second.is_bytestring());
  EXPECT_THAT(
      it->second.GetBytestring(),
      ::testing::ElementsAreArray(test_data::kCtap2MakeCredentialAuthData));

  it = attestation_object_map.find(cbor::CBORValue(kAttestationStatementKey));
  ASSERT_TRUE(it != attestation_object_map.end());
  ASSERT_TRUE(it->second.is_map());

  const auto& attestation_statement_map = it->second.GetMap();
  auto attStmt_it = attestation_statement_map.find(cbor::CBORValue("alg"));

  ASSERT_TRUE(attStmt_it != attestation_statement_map.end());
  ASSERT_TRUE(attStmt_it->second.is_unsigned());
  EXPECT_EQ(attStmt_it->second.GetUnsigned(), 7u);

  attStmt_it = attestation_statement_map.find(cbor::CBORValue("sig"));
  ASSERT_TRUE(attStmt_it != attestation_statement_map.end());
  ASSERT_TRUE(attStmt_it->second.is_bytestring());
  EXPECT_THAT(
      attStmt_it->second.GetBytestring(),
      ::testing::ElementsAreArray(test_data::kCtap2MakeCredentialSignature));

  attStmt_it = attestation_statement_map.find(cbor::CBORValue("x5c"));
  ASSERT_TRUE(attStmt_it != attestation_statement_map.end());
  const auto& certificate = attStmt_it->second;
  ASSERT_TRUE(certificate.is_array());
  ASSERT_EQ(certificate.GetArray().size(), 1u);
  ASSERT_TRUE(certificate.GetArray()[0].is_bytestring());
  EXPECT_THAT(
      certificate.GetArray()[0].GetBytestring(),
      ::testing::ElementsAreArray(test_data::kCtap2MakeCredentialCertificate));
  EXPECT_THAT(
      make_credential_response->raw_credential_id(),
      ::testing::ElementsAreArray(test_data::kCtap2MakeCredentialCredentialId));
}

TEST(CTAPResponseTest, TestMakeCredentialNoneAttestationResponse) {
  auto make_credential_response =
      ReadCTAPMakeCredentialResponse(test_data::kDeviceMakeCredentialResponse);
  ASSERT_TRUE(make_credential_response);
  make_credential_response->EraseAttestationStatement();
  EXPECT_THAT(make_credential_response->GetCBOREncodedAttestationObject(),
              ::testing::ElementsAreArray(test_data::kNoneAttestationResponse));
}

// Leveraging example 5 of section 6.1 of the CTAP spec.
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html
TEST(CTAPResponseTest, TestReadGetAssertionResponse) {
  auto get_assertion_response =
      ReadCTAPGetAssertionResponse(test_data::kDeviceGetAssertionResponse);
  ASSERT_TRUE(get_assertion_response);
  ASSERT_TRUE(get_assertion_response->num_credentials());
  EXPECT_EQ(*get_assertion_response->num_credentials(), 1u);

  EXPECT_THAT(
      get_assertion_response->auth_data().SerializeToByteArray(),
      ::testing::ElementsAreArray(test_data::kCtap2GetAssertionAuthData));
  EXPECT_THAT(
      get_assertion_response->signature(),
      ::testing::ElementsAreArray(test_data::kCtap2GetAssertionSignature));
}

// Test that U2F register response is properly parsed.
TEST(CTAPResponseTest, TestParseRegisterResponseData) {
  auto response =
      AuthenticatorMakeCredentialResponse::CreateFromU2fRegisterResponse(
          fido_parsing_utils::Materialize(test_data::kApplicationParameter),
          test_data::kTestU2fRegisterResponse);
  ASSERT_TRUE(response);
  EXPECT_THAT(response->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
  EXPECT_EQ(GetTestAttestationObjectBytes(),
            response->GetCBOREncodedAttestationObject());
}

// These test the parsing of the U2F raw bytes of the registration response.
// Test that an EC public key serializes to CBOR properly.
TEST(CTAPResponseTest, TestSerializedPublicKey) {
  auto public_key = ECPublicKey::ExtractFromU2fRegistrationResponse(
      fido_parsing_utils::kEs256, test_data::kTestU2fRegisterResponse);
  ASSERT_TRUE(public_key);
  EXPECT_THAT(public_key->EncodeAsCOSEKey(),
              ::testing::ElementsAreArray(test_data::kTestECPublicKeyCOSE));
}

// Test that the attestation statement cbor map is constructed properly.
TEST(CTAPResponseTest, TestParseU2fAttestationStatementCBOR) {
  auto fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(
          test_data::kTestU2fRegisterResponse);
  ASSERT_TRUE(fido_attestation_statement);
  auto cbor = cbor::CBORWriter::Write(
      cbor::CBORValue(fido_attestation_statement->GetAsCBORMap()));
  ASSERT_TRUE(cbor);
  EXPECT_THAT(*cbor, ::testing::ElementsAreArray(
                         test_data::kU2fAttestationStatementCBOR));
}

// Tests that well-formed attested credential data serializes properly.
TEST(CTAPResponseTest, TestSerializeAttestedCredentialData) {
  auto public_key = ECPublicKey::ExtractFromU2fRegistrationResponse(
      fido_parsing_utils::kEs256, test_data::kTestU2fRegisterResponse);
  auto attested_data = AttestedCredentialData::CreateFromU2fRegisterResponse(
      test_data::kTestU2fRegisterResponse, std::move(public_key));
  ASSERT_TRUE(attested_data);
  EXPECT_EQ(GetTestAttestedCredentialDataBytes(),
            attested_data->SerializeAsBytes());
}

// Tests that well-formed authenticator data serializes properly.
TEST(CTAPResponseTest, TestSerializeAuthenticatorData) {
  auto public_key = ECPublicKey::ExtractFromU2fRegistrationResponse(
      fido_parsing_utils::kEs256, test_data::kTestU2fRegisterResponse);
  auto attested_data = AttestedCredentialData::CreateFromU2fRegisterResponse(
      test_data::kTestU2fRegisterResponse, std::move(public_key));

  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserPresence) |
      static_cast<uint8_t>(AuthenticatorData::Flag::kAttestation);

  AuthenticatorData authenticator_data(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter), flags,
      std::vector<uint8_t>(4) /* counter */, std::move(attested_data));

  EXPECT_EQ(GetTestAuthenticatorDataBytes(),
            authenticator_data.SerializeToByteArray());
}

// Tests that a U2F attestation object serializes properly.
TEST(CTAPResponseTest, TestSerializeU2fAttestationObject) {
  auto public_key = ECPublicKey::ExtractFromU2fRegistrationResponse(
      fido_parsing_utils::kEs256, test_data::kTestU2fRegisterResponse);
  auto attested_data = AttestedCredentialData::CreateFromU2fRegisterResponse(
      test_data::kTestU2fRegisterResponse, std::move(public_key));

  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserPresence) |
      static_cast<uint8_t>(AuthenticatorData::Flag::kAttestation);
  AuthenticatorData authenticator_data(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter), flags,
      std::vector<uint8_t>(4) /* counter */, std::move(attested_data));

  // Construct the attestation statement.
  auto fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(
          test_data::kTestU2fRegisterResponse);

  // Construct the attestation object.
  auto attestation_object = std::make_unique<AttestationObject>(
      std::move(authenticator_data), std::move(fido_attestation_statement));

  ASSERT_TRUE(attestation_object);
  EXPECT_EQ(GetTestAttestationObjectBytes(),
            attestation_object->SerializeToCBOREncodedBytes());
}

// Tests that U2F authenticator data is properly serialized.
TEST(CTAPResponseTest, TestSerializeAuthenticatorDataForSign) {
  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserPresence);

  EXPECT_THAT(
      AuthenticatorData(
          fido_parsing_utils::Materialize(test_data::kApplicationParameter),
          flags, GetTestSignatureCounter(), base::nullopt)
          .SerializeToByteArray(),
      ::testing::ElementsAreArray(test_data::kTestSignAuthenticatorData));
}

TEST(CTAPResponseTest, TestParseSignResponseData) {
  auto response = AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter),
      GetTestSignResponse(), GetTestCredentialRawIdBytes());
  ASSERT_TRUE(response);
  EXPECT_EQ(GetTestCredentialRawIdBytes(), response->raw_credential_id());
  EXPECT_THAT(
      response->auth_data().SerializeToByteArray(),
      ::testing::ElementsAreArray(test_data::kTestSignAuthenticatorData));
  EXPECT_THAT(response->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));
}

TEST(CTAPResponseTest, TestParseU2fSignWithNullNullKeyHandle) {
  auto response = AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter),
      GetTestSignResponse(), std::vector<uint8_t>());
  EXPECT_FALSE(response);
}

TEST(CTAPResponseTest, TestParseU2fSignWithNullResponse) {
  auto response = AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter),
      std::vector<uint8_t>(), GetTestCredentialRawIdBytes());
  EXPECT_FALSE(response);
}

TEST(CTAPResponseTest, TestParseU2fSignWithNullCorruptedCounter) {
  // A sign response of less than 5 bytes.
  auto response = AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter),
      GetTestCorruptedSignResponse(3), GetTestCredentialRawIdBytes());
  EXPECT_FALSE(response);
}

TEST(CTAPResponseTest, TestParseU2fSignWithNullCorruptedSignature) {
  // A sign response no more than 5 bytes.
  auto response = AuthenticatorGetAssertionResponse::CreateFromU2fSignResponse(
      fido_parsing_utils::Materialize(test_data::kApplicationParameter),
      GetTestCorruptedSignResponse(5), GetTestCredentialRawIdBytes());
  EXPECT_FALSE(response);
}

}  // namespace device
