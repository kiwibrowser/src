// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_command_constructor.h"

#include <utility>

#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

CtapMakeCredentialRequest ConstructMakeCredentialRequest() {
  PublicKeyCredentialRpEntity rp("acme.com");
  rp.SetRpName("acme.com");

  PublicKeyCredentialUserEntity user(
      fido_parsing_utils::Materialize(test_data::kUserId));
  user.SetUserName("johnpsmith@example.com")
      .SetDisplayName("John P. Smith")
      .SetIconUrl(GURL("https://pics.acme.com/00/p/aBjjjpqPb.png"));

  return CtapMakeCredentialRequest(
      fido_parsing_utils::Materialize(test_data::kClientDataHash),
      std::move(rp), std::move(user),
      PublicKeyCredentialParams(PublicKeyCredentialParams(
          std::vector<PublicKeyCredentialParams::CredentialInfo>(1))));
}

CtapGetAssertionRequest ConstructGetAssertionRequest() {
  return CtapGetAssertionRequest(
      "acme.com", fido_parsing_utils::Materialize(test_data::kClientDataHash));
}

}  // namespace

TEST(U2fCommandConstructorTest, TestCreateU2fRegisterCommand) {
  const auto& register_command_without_individual_attestation =
      ConstructU2fRegisterCommand(test_data::kApplicationParameter,
                                  test_data::kChallengeParameter,
                                  false /* is_individual_attestation */);

  ASSERT_TRUE(register_command_without_individual_attestation);
  EXPECT_THAT(*register_command_without_individual_attestation,
              ::testing::ElementsAreArray(test_data::kU2fRegisterCommandApdu));

  const auto& register_command_with_individual_attestation =
      ConstructU2fRegisterCommand(test_data::kApplicationParameter,
                                  test_data::kChallengeParameter,
                                  true /* is_individual_attestation */);

  ASSERT_TRUE(register_command_with_individual_attestation);
  EXPECT_THAT(*register_command_with_individual_attestation,
              ::testing::ElementsAreArray(
                  test_data::kU2fRegisterCommandApduWithIndividualAttestation));
}

TEST(U2fCommandConstructorTest, TestCreateRegisterWithIncorrectParameters) {
  std::vector<uint8_t> application_parameter(kU2fParameterLength, 0x01);
  std::vector<uint8_t> challenge_parameter(kU2fParameterLength, 0xff);

  const auto& register_command_without_individual_attestation =
      ConstructU2fRegisterCommand(application_parameter, challenge_parameter,
                                  false /* is_individual_attestation */);

  ASSERT_TRUE(register_command_without_individual_attestation);
  ASSERT_LE(3u, register_command_without_individual_attestation->size());
  // Individual attestation bit should be cleared.
  EXPECT_EQ(0, (*register_command_without_individual_attestation)[2] & 0x80);

  const auto register_request_with_individual_attestation =
      ConstructU2fRegisterCommand(application_parameter, challenge_parameter,
                                  true /* is_individual_attestation */);

  ASSERT_TRUE(register_request_with_individual_attestation);
  ASSERT_LE(3u, register_request_with_individual_attestation->size());
  // Individual attestation bit should be set.
  EXPECT_EQ(0x80, (*register_request_with_individual_attestation)[2] & 0x80);

  // Expect null result with incorrectly sized application_parameter.
  application_parameter.push_back(0xff);
  auto incorrect_register_cmd =
      ConstructU2fRegisterCommand(application_parameter, challenge_parameter,
                                  false /* is_individual_attestation */);

  EXPECT_FALSE(incorrect_register_cmd);
  application_parameter.pop_back();

  // Expect null result with incorrectly sized challenge.
  challenge_parameter.push_back(0xff);
  incorrect_register_cmd =
      ConstructU2fRegisterCommand(application_parameter, challenge_parameter,
                                  false /* is_individual_attestation */);

  EXPECT_FALSE(incorrect_register_cmd);
}

TEST(U2fCommandConstructorTest, TestConvertCtapMakeCredentialToU2fRegister) {
  const auto make_credential_param = ConstructMakeCredentialRequest();

  EXPECT_TRUE(IsConvertibleToU2fRegisterCommand(make_credential_param));

  const auto u2f_register_command =
      ConvertToU2fRegisterCommand(make_credential_param);
  ASSERT_TRUE(u2f_register_command);
  EXPECT_THAT(*u2f_register_command,
              ::testing::ElementsAreArray(test_data::kU2fRegisterCommandApdu));
}

TEST(U2fCommandConstructorTest,
     TestConvertCtapMakeCredentialToU2fCheckOnlySign) {
  auto make_credential_param = ConstructMakeCredentialRequest();
  PublicKeyCredentialDescriptor credential_descriptor(
      CredentialType::kPublicKey,
      fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle));
  std::vector<PublicKeyCredentialDescriptor> exclude_list;
  exclude_list.push_back(credential_descriptor);
  make_credential_param.SetExcludeList(std::move(exclude_list));
  EXPECT_TRUE(IsConvertibleToU2fRegisterCommand(make_credential_param));

  const auto u2f_check_only_sign = ConvertToU2fCheckOnlySignCommand(
      make_credential_param, credential_descriptor);

  ASSERT_TRUE(u2f_check_only_sign);
  EXPECT_THAT(
      *u2f_check_only_sign,
      ::testing::ElementsAreArray(test_data::kU2fCheckOnlySignCommandApdu));
}

TEST(U2fCommandConstructorTest,
     TestConvertCtapMakeCredentialToU2fCheckOnlySignWithInvalidCredentialType) {
  auto make_credential_param = ConstructMakeCredentialRequest();
  PublicKeyCredentialDescriptor credential_descriptor(
      // Purposefully construct an invalid CredentialType.
      static_cast<CredentialType>(-1),
      fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle));
  std::vector<PublicKeyCredentialDescriptor> exclude_list;
  exclude_list.push_back(credential_descriptor);
  make_credential_param.SetExcludeList(std::move(exclude_list));
  EXPECT_TRUE(IsConvertibleToU2fRegisterCommand(make_credential_param));

  const auto u2f_check_only_sign = ConvertToU2fCheckOnlySignCommand(
      make_credential_param, credential_descriptor);

  EXPECT_FALSE(u2f_check_only_sign);
}

TEST(U2fCommandConstructorTest, TestU2fRegisterCredentialAlgorithmRequirement) {
  PublicKeyCredentialRpEntity rp("acme.com");
  rp.SetRpName("acme.com");

  PublicKeyCredentialUserEntity user(
      fido_parsing_utils::Materialize(test_data::kUserId));
  user.SetUserName("johnpsmith@example.com")
      .SetDisplayName("John P. Smith")
      .SetIconUrl(GURL("https://pics.acme.com/00/p/aBjjjpqPb.png"));

  CtapMakeCredentialRequest make_credential_param(
      fido_parsing_utils::Materialize(test_data::kClientDataHash),
      std::move(rp), std::move(user),
      PublicKeyCredentialParams({{CredentialType::kPublicKey, -257}}));

  EXPECT_FALSE(IsConvertibleToU2fRegisterCommand(make_credential_param));
}

TEST(U2fCommandConstructorTest, TestU2fRegisterUserVerificationRequirement) {
  auto make_credential_param = ConstructMakeCredentialRequest();
  make_credential_param.SetUserVerificationRequired(true);

  EXPECT_FALSE(IsConvertibleToU2fRegisterCommand(make_credential_param));
}

TEST(U2fCommandConstructorTest, TestU2fRegisterResidentKeyRequirement) {
  auto make_credential_param = ConstructMakeCredentialRequest();
  make_credential_param.SetResidentKeySupported(true);

  EXPECT_FALSE(IsConvertibleToU2fRegisterCommand(make_credential_param));
}

TEST(U2fCommandConstructorTest, TestCreateSignApduCommand) {
  const auto& encoded_sign = ConstructU2fSignCommand(
      test_data::kApplicationParameter, test_data::kChallengeParameter,
      test_data::kU2fSignKeyHandle);
  ASSERT_TRUE(encoded_sign);
  EXPECT_THAT(*encoded_sign,
              ::testing::ElementsAreArray(test_data::kU2fSignCommandApdu));

  const auto encoded_sign_check_only = ConstructU2fSignCommand(
      test_data::kApplicationParameter, test_data::kChallengeParameter,
      test_data::kU2fSignKeyHandle, true /* check_only */);
  ASSERT_TRUE(encoded_sign_check_only);
  EXPECT_THAT(
      *encoded_sign_check_only,
      ::testing::ElementsAreArray(test_data::kU2fCheckOnlySignCommandApdu));
}

TEST(U2fCommandConstructorTest, TestConvertCtapGetAssertionToU2fSignRequest) {
  auto get_assertion_req = ConstructGetAssertionRequest();
  std::vector<PublicKeyCredentialDescriptor> allowed_list;
  allowed_list.push_back(PublicKeyCredentialDescriptor(
      CredentialType::kPublicKey,
      fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle)));
  get_assertion_req.SetAllowList(std::move(allowed_list));

  const auto u2f_sign_command = ConvertToU2fSignCommand(
      get_assertion_req, ApplicationParameterType::kPrimary,
      fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle));

  EXPECT_TRUE(IsConvertibleToU2fSignCommand(get_assertion_req));
  ASSERT_TRUE(u2f_sign_command);
  EXPECT_THAT(*u2f_sign_command,
              ::testing::ElementsAreArray(test_data::kU2fSignCommandApdu));
}

TEST(U2fCommandConstructorTest, TestU2fSignAllowListRequirement) {
  auto get_assertion_req = ConstructGetAssertionRequest();
  EXPECT_FALSE(IsConvertibleToU2fSignCommand(get_assertion_req));
}

TEST(U2fCommandConstructorTest, TestU2fSignUserVerificationRequirement) {
  auto get_assertion_req = ConstructGetAssertionRequest();
  std::vector<PublicKeyCredentialDescriptor> allowed_list;
  allowed_list.push_back(PublicKeyCredentialDescriptor(
      CredentialType::kPublicKey,
      fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle)));
  get_assertion_req.SetAllowList(std::move(allowed_list));
  get_assertion_req.SetUserVerification(UserVerificationRequirement::kRequired);

  EXPECT_FALSE(IsConvertibleToU2fSignCommand(get_assertion_req));
}

}  // namespace device
