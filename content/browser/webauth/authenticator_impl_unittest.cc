// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_parser.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "components/cbor/cbor_reader.h"
#include "components/cbor/cbor_values.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_features.h"
#include "content/public/test/test_service_manager_context.h"
#include "content/test/test_render_frame_host.h"
#include "device/fido/fake_hid_impl_for_testing.h"
#include "device/fido/scoped_virtual_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using ::testing::_;

using cbor::CBORValue;
using cbor::CBORReader;
using webauth::mojom::AttestationConveyancePreference;
using webauth::mojom::AuthenticatorPtr;
using webauth::mojom::AuthenticatorSelectionCriteria;
using webauth::mojom::AuthenticatorSelectionCriteriaPtr;
using webauth::mojom::AuthenticatorStatus;
using webauth::mojom::GetAssertionAuthenticatorResponsePtr;
using webauth::mojom::MakeCredentialAuthenticatorResponsePtr;
using webauth::mojom::PublicKeyCredentialCreationOptions;
using webauth::mojom::PublicKeyCredentialCreationOptionsPtr;
using webauth::mojom::PublicKeyCredentialDescriptor;
using webauth::mojom::PublicKeyCredentialDescriptorPtr;
using webauth::mojom::PublicKeyCredentialParameters;
using webauth::mojom::PublicKeyCredentialParametersPtr;
using webauth::mojom::PublicKeyCredentialRequestOptions;
using webauth::mojom::PublicKeyCredentialRequestOptionsPtr;
using webauth::mojom::PublicKeyCredentialRpEntity;
using webauth::mojom::PublicKeyCredentialRpEntityPtr;
using webauth::mojom::PublicKeyCredentialType;
using webauth::mojom::PublicKeyCredentialUserEntity;
using webauth::mojom::PublicKeyCredentialUserEntityPtr;

namespace {

typedef struct {
  const char* origin;
  // Either a relying party ID or a U2F AppID.
  const char* claimed_authority;
} OriginClaimedAuthorityPair;

constexpr char kTestOrigin1[] = "https://a.google.com";
constexpr char kTestRelyingPartyId[] = "google.com";

// Test data. CBOR test data can be built using the given
// diagnostic strings and the utility at "http://CBOR.me/".
constexpr int32_t kCoseEs256 = -7;

constexpr uint8_t kTestChallengeBytes[] = {
    0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xEC, 0x17, 0x20, 0x2E, 0x42,
    0x50, 0x5F, 0x8E, 0xD2, 0xB1, 0x6A, 0xE2, 0x2F, 0x16, 0xBB, 0x05,
    0xB8, 0x8C, 0x25, 0xDB, 0x9E, 0x60, 0x26, 0x45, 0xF1, 0x41};

constexpr char kTestRegisterClientDataJsonString[] =
    R"({"challenge":"aHE0loIi7BcgLkJQX47SsWriLxa7BbiMJdueYCZF8UE","origin":)"
    R"("https://a.google.com", "type":"webauthn.create"})";

constexpr char kTestSignClientDataJsonString[] =
    R"({"challenge":"aHE0loIi7BcgLkJQX47SsWriLxa7BbiMJdueYCZF8UE","origin":)"
    R"("https://a.google.com", "type":"webauthn.get"})";

constexpr OriginClaimedAuthorityPair kValidRelyingPartyTestCases[] = {
    {"http://localhost", "localhost"},
    {"https://myawesomedomain", "myawesomedomain"},
    {"https://foo.bar.google.com", "foo.bar.google.com"},
    {"https://foo.bar.google.com", "bar.google.com"},
    {"https://foo.bar.google.com", "google.com"},
    {"https://earth.login.awesomecompany", "login.awesomecompany"},
    {"https://google.com:1337", "google.com"},

    // Hosts with trailing dot valid for rpIds with or without trailing dot.
    // Hosts without trailing dots only matches rpIDs without trailing dot.
    // Two trailing dots only matches rpIDs with two trailing dots.
    {"https://google.com.", "google.com"},
    {"https://google.com.", "google.com."},
    {"https://google.com..", "google.com.."},

    // Leading dots are ignored in canonicalized hosts.
    {"https://.google.com", "google.com"},
    {"https://..google.com", "google.com"},
    {"https://.google.com", ".google.com"},
    {"https://..google.com", ".google.com"},
    {"https://accounts.google.com", ".google.com"},
};

constexpr OriginClaimedAuthorityPair kInvalidRelyingPartyTestCases[] = {
    {"https://google.com", "com"},
    {"http://google.com", "google.com"},
    {"http://myawesomedomain", "myawesomedomain"},
    {"https://google.com", "foo.bar.google.com"},
    {"http://myawesomedomain", "randomdomain"},
    {"https://myawesomedomain", "randomdomain"},
    {"https://notgoogle.com", "google.com)"},
    {"https://not-google.com", "google.com)"},
    {"https://evil.appspot.com", "appspot.com"},
    {"https://evil.co.uk", "co.uk"},

    {"https://google.com", "google.com."},
    {"https://google.com", "google.com.."},
    {"https://google.com", ".google.com"},
    {"https://google.com..", "google.com"},
    {"https://.com", "com."},
    {"https://.co.uk", "co.uk."},

    {"https://1.2.3", "1.2.3"},
    {"https://1.2.3", "2.3"},

    {"https://127.0.0.1", "127.0.0.1"},
    {"https://127.0.0.1", "27.0.0.1"},
    {"https://127.0.0.1", ".0.0.1"},
    {"https://127.0.0.1", "0.0.1"},

    {"https://[::127.0.0.1]", "127.0.0.1"},
    {"https://[::127.0.0.1]", "[127.0.0.1]"},

    {"https://[::1]", "1"},
    {"https://[::1]", "1]"},
    {"https://[::1]", "::1"},
    {"https://[::1]", "[::1]"},
    {"https://[1::1]", "::1"},
    {"https://[1::1]", "::1]"},
    {"https://[1::1]", "[::1]"},

    {"http://google.com:443", "google.com"},
    {"data:google.com", "google.com"},
    {"data:text/html,google.com", "google.com"},
    {"ws://google.com", "google.com"},
    {"gopher://google.com", "google.com"},
    {"ftp://google.com", "google.com"},
    {"file:///google.com", "google.com"},
    // Use of webauthn from a WSS origin may be technically valid, but we
    // prohibit use on non-HTTPS origins. (At least for now.)
    {"wss://google.com", "google.com"},

    {"data:,", ""},
    {"https://google.com", ""},
    {"ws:///google.com", ""},
    {"wss:///google.com", ""},
    {"gopher://google.com", ""},
    {"ftp://google.com", ""},
    {"file:///google.com", ""},

    // This case is acceptable according to spec, but both renderer
    // and browser handling currently do not permit it.
    {"https://login.awesomecompany", "awesomecompany"},

    // These are AppID test cases, but should also be invalid relying party
    // examples too.
    {"https://example.com", "https://com/"},
    {"https://example.com", "https://com/foo"},
    {"https://example.com", "https://foo.com/"},
    {"https://example.com", "http://example.com"},
    {"http://example.com", "https://example.com"},
    {"https://127.0.0.1", "https://127.0.0.1"},
    {"https://www.notgoogle.com",
     "https://www.gstatic.com/securitykey/origins.json"},
    {"https://www.google.com",
     "https://www.gstatic.com/securitykey/origins.json#x"},
    {"https://www.google.com",
     "https://www.gstatic.com/securitykey/origins.json2"},
    {"https://www.google.com", "https://gstatic.com/securitykey/origins.json"},
    {"https://ggoogle.com", "https://www.gstatic.com/securitykey/origi"},
    {"https://com", "https://www.gstatic.com/securitykey/origins.json"},
};

using TestMakeCredentialCallback = device::test::StatusAndValueCallbackReceiver<
    AuthenticatorStatus,
    MakeCredentialAuthenticatorResponsePtr>;
using TestGetAssertionCallback = device::test::StatusAndValueCallbackReceiver<
    AuthenticatorStatus,
    GetAssertionAuthenticatorResponsePtr>;

std::vector<uint8_t> GetTestChallengeBytes() {
  return std::vector<uint8_t>(std::begin(kTestChallengeBytes),
                              std::end(kTestChallengeBytes));
}

PublicKeyCredentialRpEntityPtr GetTestPublicKeyCredentialRPEntity() {
  auto entity = PublicKeyCredentialRpEntity::New();
  entity->id = std::string(kTestRelyingPartyId);
  entity->name = "TestRP@example.com";
  return entity;
}

PublicKeyCredentialUserEntityPtr GetTestPublicKeyCredentialUserEntity() {
  auto entity = PublicKeyCredentialUserEntity::New();
  entity->display_name = "User A. Name";
  std::vector<uint8_t> id(32, 0x0A);
  entity->id = id;
  entity->name = "username@example.com";
  entity->icon = GURL("fakeurl2.png");
  return entity;
}

std::vector<PublicKeyCredentialParametersPtr>
GetTestPublicKeyCredentialParameters(int32_t algorithm_identifier) {
  std::vector<PublicKeyCredentialParametersPtr> parameters;
  auto fake_parameter = PublicKeyCredentialParameters::New();
  fake_parameter->type = webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY;
  fake_parameter->algorithm_identifier = algorithm_identifier;
  parameters.push_back(std::move(fake_parameter));
  return parameters;
}

AuthenticatorSelectionCriteriaPtr GetTestAuthenticatorSelectionCriteria() {
  auto criteria = AuthenticatorSelectionCriteria::New();
  criteria->authenticator_attachment =
      webauth::mojom::AuthenticatorAttachment::NO_PREFERENCE;
  criteria->require_resident_key = false;
  criteria->user_verification =
      webauth::mojom::UserVerificationRequirement::PREFERRED;
  return criteria;
}

std::vector<PublicKeyCredentialDescriptorPtr> GetTestAllowCredentials() {
  std::vector<PublicKeyCredentialDescriptorPtr> descriptors;
  auto credential = PublicKeyCredentialDescriptor::New();
  credential->type = PublicKeyCredentialType::PUBLIC_KEY;
  std::vector<uint8_t> id(32, 0x0A);
  credential->id = id;
  descriptors.push_back(std::move(credential));
  return descriptors;
}

PublicKeyCredentialCreationOptionsPtr
GetTestPublicKeyCredentialCreationOptions() {
  auto options = PublicKeyCredentialCreationOptions::New();
  options->relying_party = GetTestPublicKeyCredentialRPEntity();
  options->user = GetTestPublicKeyCredentialUserEntity();
  options->public_key_parameters =
      GetTestPublicKeyCredentialParameters(kCoseEs256);
  options->challenge.assign(32, 0x0A);
  options->adjusted_timeout = base::TimeDelta::FromMinutes(1);
  options->authenticator_selection = GetTestAuthenticatorSelectionCriteria();
  return options;
}

PublicKeyCredentialRequestOptionsPtr
GetTestPublicKeyCredentialRequestOptions() {
  auto options = PublicKeyCredentialRequestOptions::New();
  options->relying_party_id = std::string(kTestRelyingPartyId);
  options->challenge.assign(32, 0x0A);
  options->adjusted_timeout = base::TimeDelta::FromMinutes(1);
  options->user_verification =
      webauth::mojom::UserVerificationRequirement::PREFERRED;
  options->allow_credentials = GetTestAllowCredentials();
  return options;
}

}  // namespace

class AuthenticatorImplTest : public content::RenderViewHostTestHarness {
 public:
  AuthenticatorImplTest() {}
  ~AuthenticatorImplTest() override {}

 protected:
  // Simulates navigating to a page and getting the page contents and language
  // for that navigation.
  void SimulateNavigation(const GURL& url) {
    if (main_rfh()->GetLastCommittedURL() != url)
      NavigateAndCommit(url);
  }

  AuthenticatorPtr ConnectToAuthenticator() {
    authenticator_impl_ = std::make_unique<AuthenticatorImpl>(main_rfh());
    AuthenticatorPtr authenticator;
    authenticator_impl_->Bind(mojo::MakeRequest(&authenticator));
    return authenticator;
  }

  AuthenticatorPtr ConnectToAuthenticator(
      service_manager::Connector* connector,
      std::unique_ptr<base::OneShotTimer> timer) {
    authenticator_impl_.reset(
        new AuthenticatorImpl(main_rfh(), connector, std::move(timer)));
    AuthenticatorPtr authenticator;
    authenticator_impl_->Bind(mojo::MakeRequest(&authenticator));
    return authenticator;
  }

  url::Origin GetTestOrigin() {
    const GURL test_relying_party_url(kTestOrigin1);
    CHECK(test_relying_party_url.is_valid());
    return url::Origin::Create(test_relying_party_url);
  }

  std::string GetTestClientDataJSON(std::string type) {
    return AuthenticatorImpl::SerializeCollectedClientDataToJson(
        std::move(type), GetTestOrigin(), GetTestChallengeBytes(),
        base::nullopt);
  }

  std::string GetTokenBindingTestClientDataJSON(
      base::Optional<base::span<const uint8_t>> token_binding) {
    return AuthenticatorImpl::SerializeCollectedClientDataToJson(
        client_data::kGetType, GetTestOrigin(), GetTestChallengeBytes(),
        token_binding);
  }

  AuthenticatorStatus TryAuthenticationWithAppId(const std::string& origin,
                                                 const std::string& appid) {
    const GURL origin_url(origin);
    NavigateAndCommit(origin_url);
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();
    options->relying_party_id = origin_url.host();
    options->appid = appid;

    TestGetAssertionCallback callback_receiver;
    authenticator->GetAssertion(std::move(options),
                                callback_receiver.callback());
    callback_receiver.WaitForCallback();

    return callback_receiver.status();
  }

  bool SupportsTransportProtocol(::device::FidoTransportProtocol protocol) {
    return base::ContainsKey(authenticator_impl_->protocols_, protocol);
  }

 private:
  std::unique_ptr<AuthenticatorImpl> authenticator_impl_;
};

// Verify behavior for various combinations of origins and RP IDs.
TEST_F(AuthenticatorImplTest, MakeCredentialOriginAndRpIds) {
  // These instances should return security errors (for circumstances
  // that would normally crash the renderer).
  for (auto test_case : kInvalidRelyingPartyTestCases) {
    SCOPED_TRACE(std::string(test_case.claimed_authority) + " " +
                 std::string(test_case.origin));

    NavigateAndCommit(GURL(test_case.origin));
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialCreationOptionsPtr options =
        GetTestPublicKeyCredentialCreationOptions();
    options->relying_party->id = test_case.claimed_authority;
    TestMakeCredentialCallback callback_receiver;
    authenticator->MakeCredential(std::move(options),
                                  callback_receiver.callback());
    callback_receiver.WaitForCallback();
    EXPECT_EQ(AuthenticatorStatus::INVALID_DOMAIN, callback_receiver.status());
  }

  // These instances pass the origin and relying party checks and return at
  // the algorithm check.
  for (auto test_case : kValidRelyingPartyTestCases) {
    SCOPED_TRACE(std::string(test_case.claimed_authority) + " " +
                 std::string(test_case.origin));

    NavigateAndCommit(GURL(test_case.origin));
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialCreationOptionsPtr options =
        GetTestPublicKeyCredentialCreationOptions();
    options->relying_party->id = test_case.claimed_authority;
    options->public_key_parameters = GetTestPublicKeyCredentialParameters(123);

    TestMakeCredentialCallback callback_receiver;
    authenticator->MakeCredential(std::move(options),
                                  callback_receiver.callback());
    callback_receiver.WaitForCallback();
    EXPECT_EQ(AuthenticatorStatus::ALGORITHM_UNSUPPORTED,
              callback_receiver.status());
  }
}

// Test that service returns ALGORITHM_UNSUPPORTED if no parameters contain
// a supported algorithm.
TEST_F(AuthenticatorImplTest, MakeCredentialNoSupportedAlgorithm) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  options->public_key_parameters = GetTestPublicKeyCredentialParameters(123);

  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::ALGORITHM_UNSUPPORTED,
            callback_receiver.status());
}

// Test that service returns USER_VERIFICATION_UNSUPPORTED if user verification
// is REQUIRED for get().
TEST_F(AuthenticatorImplTest, GetAssertionUserVerification) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  options->user_verification =
      webauth::mojom::UserVerificationRequirement::REQUIRED;
  TestGetAssertionCallback callback_receiver;
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::USER_VERIFICATION_UNSUPPORTED,
            callback_receiver.status());
}

// Test that service returns AUTHENTICATOR_CRITERIA_UNSUPPORTED if user
// verification is REQUIRED for create().
TEST_F(AuthenticatorImplTest, MakeCredentialUserVerification) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  options->authenticator_selection->user_verification =
      webauth::mojom::UserVerificationRequirement::REQUIRED;

  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::AUTHENTICATOR_CRITERIA_UNSUPPORTED,
            callback_receiver.status());
}

// Test that service returns AUTHENTICATOR_CRITERIA_UNSUPPORTED if resident key
// is requested for create().
TEST_F(AuthenticatorImplTest, MakeCredentialResidentKey) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  options->authenticator_selection->require_resident_key = true;

  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::AUTHENTICATOR_CRITERIA_UNSUPPORTED,
            callback_receiver.status());
}

// Test that service returns AUTHENTICATOR_CRITERIA_UNSUPPORTED if a platform
// authenticator is requested for U2F.
TEST_F(AuthenticatorImplTest, MakeCredentialPlatformAuthenticator) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  options->authenticator_selection->authenticator_attachment =
      webauth::mojom::AuthenticatorAttachment::PLATFORM;

  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::AUTHENTICATOR_CRITERIA_UNSUPPORTED,
            callback_receiver.status());
}

// Parses its arguments as JSON and expects that all the keys in the first are
// also in the second, and with the same value.
void CheckJSONIsSubsetOfJSON(base::StringPiece subset_str,
                             base::StringPiece test_str) {
  std::unique_ptr<base::Value> subset(base::JSONReader::Read(subset_str));
  ASSERT_TRUE(subset);
  ASSERT_TRUE(subset->is_dict());
  std::unique_ptr<base::Value> test(base::JSONReader::Read(test_str));
  ASSERT_TRUE(test);
  ASSERT_TRUE(test->is_dict());

  for (const auto& item : subset->DictItems()) {
    base::Value* test_value = test->FindKey(item.first);
    if (test_value == nullptr) {
      ADD_FAILURE() << item.first << " does not exist in the test dictionary";
      continue;
    }

    if (!item.second.Equals(test_value)) {
      std::string want, got;
      ASSERT_TRUE(base::JSONWriter::Write(item.second, &want));
      ASSERT_TRUE(base::JSONWriter::Write(*test_value, &got));
      ADD_FAILURE() << "Value of " << item.first << " is unequal: want " << want
                    << " got " << got;
    }
  }
}

// Test that client data serializes to JSON properly.
TEST_F(AuthenticatorImplTest, TestSerializedRegisterClientData) {
  CheckJSONIsSubsetOfJSON(kTestRegisterClientDataJsonString,
                          GetTestClientDataJSON(client_data::kCreateType));
}

TEST_F(AuthenticatorImplTest, TestSerializedSignClientData) {
  CheckJSONIsSubsetOfJSON(kTestSignClientDataJsonString,
                          GetTestClientDataJSON(client_data::kGetType));
}

TEST_F(AuthenticatorImplTest, TestTokenBindingClientData) {
  const std::vector<
      std::pair<base::Optional<std::vector<uint8_t>>, const char*>>
      kTestCases = {
          std::make_pair(base::nullopt, ""),
          std::make_pair(std::vector<uint8_t>{},
                         R"({"tokenBinding":{"status":"supported"}})"),
          std::make_pair(
              std::vector<uint8_t>{1, 2, 3, 4},
              R"({"tokenBinding":{"status":"present","id":"AQIDBA"}})"),
      };

  for (const auto& test : kTestCases) {
    const auto& token_binding = test.first;
    const std::string expected_json_subset = test.second;
    SCOPED_TRACE(expected_json_subset);
    const std::string client_data =
        GetTokenBindingTestClientDataJSON(token_binding);

    if (!expected_json_subset.empty()) {
      CheckJSONIsSubsetOfJSON(expected_json_subset, client_data);
    } else {
      EXPECT_TRUE(client_data.find("tokenBinding") == std::string::npos)
          << client_data;
    }
  }
}

TEST_F(AuthenticatorImplTest, TestMakeCredentialTimeout) {
  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  TestMakeCredentialCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      service_manager::Identity(device::mojom::kServiceName),
      device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
}

// Verify behavior for various combinations of origins and RP IDs.
TEST_F(AuthenticatorImplTest, GetAssertionOriginAndRpIds) {
  // These instances should return security errors (for circumstances
  // that would normally crash the renderer).
  for (const OriginClaimedAuthorityPair& test_case :
       kInvalidRelyingPartyTestCases) {
    SCOPED_TRACE(std::string(test_case.claimed_authority) + " " +
                 std::string(test_case.origin));

    NavigateAndCommit(GURL(test_case.origin));
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();
    options->relying_party_id = test_case.claimed_authority;

    TestGetAssertionCallback callback_receiver;
    authenticator->GetAssertion(std::move(options),
                                callback_receiver.callback());
    callback_receiver.WaitForCallback();
    EXPECT_EQ(AuthenticatorStatus::INVALID_DOMAIN, callback_receiver.status());
  }
}

constexpr OriginClaimedAuthorityPair kValidAppIdCases[] = {
    {"https://example.com", "https://example.com"},
    {"https://www.example.com", "https://example.com"},
    {"https://example.com", "https://www.example.com"},
    {"https://example.com", "https://foo.bar.example.com"},
    {"https://example.com", "https://foo.bar.example.com/foo/bar"},
    {"https://google.com", "https://www.gstatic.com/securitykey/origins.json"},
    {"https://www.google.com",
     "https://www.gstatic.com/securitykey/origins.json"},
    {"https://www.google.com",
     "https://www.gstatic.com/securitykey/a/google.com/origins.json"},
    {"https://accounts.google.com",
     "https://www.gstatic.com/securitykey/origins.json"},
};

// Verify behavior for various combinations of origins and RP IDs.
TEST_F(AuthenticatorImplTest, AppIdExtension) {
  TestServiceManagerContext smc;
  device::test::ScopedVirtualFidoDevice virtual_device;

  for (const auto& test_case : kValidAppIdCases) {
    SCOPED_TRACE(std::string(test_case.origin) + " " +
                 std::string(test_case.claimed_authority));

    EXPECT_EQ(AuthenticatorStatus::CREDENTIAL_NOT_RECOGNIZED,
              TryAuthenticationWithAppId(test_case.origin,
                                         test_case.claimed_authority));
  }

  // All the invalid relying party test cases should also be invalid as AppIDs.
  for (const auto& test_case : kInvalidRelyingPartyTestCases) {
    SCOPED_TRACE(std::string(test_case.origin) + " " +
                 std::string(test_case.claimed_authority));

    if (strlen(test_case.claimed_authority) == 0) {
      // In this case, no AppID is actually being tested.
      continue;
    }

    EXPECT_EQ(AuthenticatorStatus::INVALID_DOMAIN,
              TryAuthenticationWithAppId(test_case.origin,
                                         test_case.claimed_authority));
  }
}

TEST_F(AuthenticatorImplTest, TestGetAssertionTimeout) {
  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      service_manager::Identity(device::mojom::kServiceName),
      device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, OversizedCredentialId) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  // 255 is the maximum size of a U2F credential ID. We also test one greater
  // (256) to ensure that nothing untoward happens.
  const std::vector<size_t> kSizes = {255, 256};

  for (const size_t size : kSizes) {
    SCOPED_TRACE(size);

    SimulateNavigation(GURL(kTestOrigin1));
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();
    auto credential = PublicKeyCredentialDescriptor::New();
    credential->type = PublicKeyCredentialType::PUBLIC_KEY;
    credential->id.resize(size);

    const bool should_be_valid = size < 256;
    if (should_be_valid) {
      ASSERT_TRUE(scoped_virtual_device.mutable_state()->InjectRegistration(
          credential->id, kTestRelyingPartyId));
    }

    options->allow_credentials.emplace_back(std::move(credential));

    TestGetAssertionCallback callback_receiver;
    authenticator->GetAssertion(std::move(options),
                                callback_receiver.callback());
    callback_receiver.WaitForCallback();

    if (should_be_valid) {
      EXPECT_EQ(AuthenticatorStatus::SUCCESS, callback_receiver.status());
    } else {
      EXPECT_EQ(AuthenticatorStatus::CREDENTIAL_NOT_RECOGNIZED,
                callback_receiver.status());
    }
  }
}

TEST_F(AuthenticatorImplTest, TestCableDiscoveryEnabledWithSwitch) {
  TestServiceManagerContext service_manager_context;
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      std::vector<base::Feature>{features::kWebAuthCtap2,
                                 features::kWebAuthCable},
      std::vector<base::Feature>{});

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      service_manager::Identity(device::mojom::kServiceName),
      device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
  EXPECT_TRUE(SupportsTransportProtocol(
      device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy));
}

TEST_F(AuthenticatorImplTest, TestCableDiscoveryDisabledForMakeCredential) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      std::vector<base::Feature>{features::kWebAuthCtap2,
                                 features::kWebAuthCable},
      std::vector<base::Feature>{});

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  TestMakeCredentialCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      service_manager::Identity(device::mojom::kServiceName),
      device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
  EXPECT_FALSE(SupportsTransportProtocol(
      device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy));
}

TEST_F(AuthenticatorImplTest, TestCableDiscoveryDisabledWithoutSwitch) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kWebAuthCtap2);

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      service_manager::Identity(device::mojom::kServiceName),
      device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
  EXPECT_FALSE(SupportsTransportProtocol(
      device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy));
}

TEST_F(AuthenticatorImplTest, TestU2fDeviceDoesNotSupportMakeCredential) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kWebAuthCtap2);

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  TestMakeCredentialCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  device::test::ScopedVirtualFidoDevice virtual_device;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, TestU2fDeviceDoesNotSupportGetAssertion) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kWebAuthCtap2);

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  device::test::ScopedVirtualFidoDevice virtual_device;
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();
  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, Ctap2AcceptsEmptyAllowCredentials) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kWebAuthCtap2);

  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  options->allow_credentials.clear();
  TestGetAssertionCallback callback_receiver;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  device::test::ScopedVirtualFidoDevice virtual_device;
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  callback_receiver.WaitForCallback();
  // Doesn't error out with EMPTY_ALLOW_CREDENTIALS but continues to a
  // NOT_ALLOWED_ERROR.
  EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR, callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, GetAssertionWithEmptyAllowCredentials) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  options->allow_credentials.clear();

  TestGetAssertionCallback callback_receiver;
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());
  callback_receiver.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::EMPTY_ALLOW_CREDENTIALS,
            callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, MakeCredentialAlreadyRegistered) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();
  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();

  // Exclude the one already registered credential.
  options->exclude_credentials = GetTestAllowCredentials();
  ASSERT_TRUE(scoped_virtual_device.mutable_state()->InjectRegistration(
      options->exclude_credentials[0]->id, kTestRelyingPartyId));

  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());
  callback_receiver.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::CREDENTIAL_EXCLUDED,
            callback_receiver.status());
}

TEST_F(AuthenticatorImplTest, MakeCredentialPendingRequest) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  // Make first request.
  PublicKeyCredentialCreationOptionsPtr options =
      GetTestPublicKeyCredentialCreationOptions();
  TestMakeCredentialCallback callback_receiver;
  authenticator->MakeCredential(std::move(options),
                                callback_receiver.callback());

  // Make second request.
  // TODO(crbug.com/785955): Rework to ensure there are potential race
  // conditions once we have VirtualAuthenticatorEnvironment.
  PublicKeyCredentialCreationOptionsPtr options2 =
      GetTestPublicKeyCredentialCreationOptions();
  TestMakeCredentialCallback callback_receiver2;
  authenticator->MakeCredential(std::move(options2),
                                callback_receiver2.callback());
  callback_receiver2.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::PENDING_REQUEST, callback_receiver2.status());
}

TEST_F(AuthenticatorImplTest, GetAssertionPendingRequest) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  // Make first request.
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver;
  authenticator->GetAssertion(std::move(options), callback_receiver.callback());

  // Make second request.
  // TODO(crbug.com/785955): Rework to ensure there are potential race
  // conditions once we have VirtualAuthenticatorEnvironment.
  PublicKeyCredentialRequestOptionsPtr options2 =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback callback_receiver2;
  authenticator->GetAssertion(std::move(options2),
                              callback_receiver2.callback());
  callback_receiver2.WaitForCallback();

  EXPECT_EQ(AuthenticatorStatus::PENDING_REQUEST, callback_receiver2.status());
}

TEST_F(AuthenticatorImplTest, InvalidResponse) {
  device::test::ScopedVirtualFidoDevice scoped_virtual_device;
  TestServiceManagerContext service_manager_context;

  scoped_virtual_device.mutable_state()->simulate_invalid_response = true;

  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  {
    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();
    TestGetAssertionCallback callback_receiver;
    authenticator->GetAssertion(std::move(options),
                                callback_receiver.callback());
    callback_receiver.WaitForCallback();

    EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR,
              callback_receiver.status());
  }

  {
    PublicKeyCredentialCreationOptionsPtr options =
        GetTestPublicKeyCredentialCreationOptions();
    TestMakeCredentialCallback callback_receiver;
    authenticator->MakeCredential(std::move(options),
                                  callback_receiver.callback());
    callback_receiver.WaitForCallback();

    EXPECT_EQ(AuthenticatorStatus::NOT_ALLOWED_ERROR,
              callback_receiver.status());
  }
}

enum class IndividualAttestation {
  REQUESTED,
  NOT_REQUESTED,
};

enum class AttestationConsent {
  GRANTED,
  DENIED,
};

// Implements ContentBrowserClient and allows webauthn-related calls to be
// mocked.
class AuthenticatorTestContentBrowserClient : public ContentBrowserClient {
 public:
  bool ShouldPermitIndividualAttestationForWebauthnRPID(
      content::BrowserContext* browser_context,
      const std::string& rp_id) override {
    return individual_attestation_ == IndividualAttestation::REQUESTED;
  }

  void ShouldReturnAttestationForWebauthnRPID(
      content::RenderFrameHost* rfh,
      const std::string& rp_id,
      const url::Origin& origin,
      base::OnceCallback<void(bool)> callback) override {
    std::move(callback).Run(attestation_consent_ ==
                            AttestationConsent::GRANTED);
  }

  bool IsFocused(content::WebContents* web_contents) override {
    return focused_;
  }

  void set_individual_attestation(IndividualAttestation value) {
    individual_attestation_ = value;
  }

  void set_attestation_consent(AttestationConsent value) {
    attestation_consent_ = value;
  }

  void set_focused(bool is_focused) { focused_ = is_focused; }

 private:
  IndividualAttestation individual_attestation_ =
      IndividualAttestation::NOT_REQUESTED;
  AttestationConsent attestation_consent_ = AttestationConsent::DENIED;
  bool focused_ = true;
};

// A test class that installs and removes an
// |AuthenticatorTestContentBrowserClient| automatically and can run tests
// against simulated attestation results.
class AuthenticatorContentBrowserClientTest : public AuthenticatorImplTest {
 public:
  AuthenticatorContentBrowserClientTest() = default;

  struct TestCase {
    AttestationConveyancePreference attestation_requested;
    IndividualAttestation individual_attestation;
    AttestationConsent attestation_consent;
    AuthenticatorStatus expected_status;
    const char* expected_attestation_format;
    const char* expected_certificate_substring;
  };

  void SetUp() override {
    AuthenticatorImplTest::SetUp();
    old_client_ = SetBrowserClientForTesting(&test_client_);
  }

  void TearDown() override {
    SetBrowserClientForTesting(old_client_);
    AuthenticatorImplTest::TearDown();
  }

  void RunTestCases(const std::vector<TestCase>& tests) {
    TestServiceManagerContext smc_;
    AuthenticatorPtr authenticator = ConnectToAuthenticator();

    for (size_t i = 0; i < tests.size(); i++) {
      const auto& test = tests[i];
      SCOPED_TRACE(test.attestation_consent == AttestationConsent::GRANTED
                       ? "consent granted"
                       : "consent denied");
      SCOPED_TRACE(test.individual_attestation ==
                           IndividualAttestation::REQUESTED
                       ? "individual attestation"
                       : "no individual attestation");
      SCOPED_TRACE(
          AttestationConveyancePreferenceToString(test.attestation_requested));
      SCOPED_TRACE(i);

      test_client_.set_individual_attestation(test.individual_attestation);
      test_client_.set_attestation_consent(test.attestation_consent);

      PublicKeyCredentialCreationOptionsPtr options =
          GetTestPublicKeyCredentialCreationOptions();
      options->relying_party->id = "example.com";
      options->adjusted_timeout = base::TimeDelta::FromSeconds(1);
      options->attestation = test.attestation_requested;
      TestMakeCredentialCallback callback_receiver;
      authenticator->MakeCredential(std::move(options),
                                    callback_receiver.callback());
      callback_receiver.WaitForCallback();
      ASSERT_EQ(test.expected_status, callback_receiver.status());

      if (test.expected_status != AuthenticatorStatus::SUCCESS) {
        ASSERT_STREQ("", test.expected_attestation_format);
        continue;
      }

      base::Optional<CBORValue> attestation_value =
          CBORReader::Read(callback_receiver.value()->attestation_object);
      ASSERT_TRUE(attestation_value);
      ASSERT_TRUE(attestation_value->is_map());
      const auto& attestation = attestation_value->GetMap();
      ExpectMapHasKeyWithStringValue(attestation, "fmt",
                                     test.expected_attestation_format);
      if (strlen(test.expected_certificate_substring) > 0) {
        ExpectCertificateContainingSubstring(
            attestation, test.expected_certificate_substring);
      }
    }
  }

 protected:
  AuthenticatorTestContentBrowserClient test_client_;
  device::test::ScopedVirtualFidoDevice virtual_device_;

 private:
  static const char* AttestationConveyancePreferenceToString(
      AttestationConveyancePreference v) {
    switch (v) {
      case AttestationConveyancePreference::NONE:
        return "none";
      case AttestationConveyancePreference::INDIRECT:
        return "indirect";
      case AttestationConveyancePreference::DIRECT:
        return "direct";
      default:
        NOTREACHED();
        return "";
    }
  }

  // Expects that |map| contains the given key with a string-value equal to
  // |expected|.
  static void ExpectMapHasKeyWithStringValue(const CBORValue::MapValue& map,
                                             const char* key,
                                             const char* expected) {
    const auto it = map.find(CBORValue(key));
    ASSERT_TRUE(it != map.end()) << "No such key '" << key << "'";
    const auto& value = it->second;
    EXPECT_TRUE(value.is_string())
        << "Value of '" << key << "' has type "
        << static_cast<int>(value.type()) << ", but expected to find a string";
    EXPECT_EQ(std::string(expected), value.GetString())
        << "Value of '" << key << "' is '" << value.GetString()
        << "', but expected to find '" << expected << "'";
  }

  // Asserts that the webauthn attestation CBOR map in
  // |attestation| contains a single X.509 certificate containing |substring|.
  static void ExpectCertificateContainingSubstring(
      const CBORValue::MapValue& attestation,
      const std::string& substring) {
    const auto& attestation_statement_it =
        attestation.find(CBORValue("attStmt"));
    ASSERT_TRUE(attestation_statement_it != attestation.end());
    ASSERT_TRUE(attestation_statement_it->second.is_map());
    const auto& attestation_statement =
        attestation_statement_it->second.GetMap();
    const auto& x5c_it = attestation_statement.find(CBORValue("x5c"));
    ASSERT_TRUE(x5c_it != attestation_statement.end());
    ASSERT_TRUE(x5c_it->second.is_array());
    const auto& x5c = x5c_it->second.GetArray();
    ASSERT_EQ(1u, x5c.size());
    ASSERT_TRUE(x5c[0].is_bytestring());
    base::StringPiece cert = x5c[0].GetBytestringAsString();
    EXPECT_TRUE(cert.find(substring) != cert.npos);
  }

  ContentBrowserClient* old_client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorContentBrowserClientTest);
};

TEST_F(AuthenticatorContentBrowserClientTest, AttestationBehaviour) {
  const char kStandardCommonName[] = "U2F Attestation";
  const char kIndividualCommonName[] = "Individual Cert";

  const std::vector<TestCase> kTests = {
      {
          AttestationConveyancePreference::NONE,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::SUCCESS, "none", "",
      },
      {
          AttestationConveyancePreference::NONE,
          IndividualAttestation::REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::SUCCESS, "none", "",
      },
      {
          AttestationConveyancePreference::INDIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::NOT_ALLOWED_ERROR, "", "",
      },
      {
          AttestationConveyancePreference::INDIRECT,
          IndividualAttestation::REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::NOT_ALLOWED_ERROR, "", "",
      },
      {
          AttestationConveyancePreference::INDIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS, "fido-u2f", kStandardCommonName,
      },
      {
          AttestationConveyancePreference::INDIRECT,
          IndividualAttestation::REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS, "fido-u2f", kIndividualCommonName,
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::NOT_ALLOWED_ERROR, "", "",
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::NOT_ALLOWED_ERROR, "", "",
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS, "fido-u2f", kStandardCommonName,
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS, "fido-u2f", kIndividualCommonName,
      },
  };

  virtual_device_.mutable_state()->attestation_cert_common_name =
      kStandardCommonName;
  virtual_device_.mutable_state()->individual_attestation_cert_common_name =
      kIndividualCommonName;
  NavigateAndCommit(GURL("https://example.com"));

  RunTestCases(kTests);
}

TEST_F(AuthenticatorContentBrowserClientTest,
       InappropriatelyIdentifyingAttestation) {
  // This common name is used by several devices that have inappropriately
  // identifying attestation certificates.
  const char kCommonName[] = "FT FIDO 0100";

  const std::vector<TestCase> kTests = {
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::DENIED,
          AuthenticatorStatus::NOT_ALLOWED_ERROR, "", "",
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::NOT_REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS,
          // If individual attestation was not requested then the attestation
          // certificate will be removed, even if consent is given, because
          // the consent isn't to be tracked.
          "none", "",
      },
      {
          AttestationConveyancePreference::DIRECT,
          IndividualAttestation::REQUESTED, AttestationConsent::GRANTED,
          AuthenticatorStatus::SUCCESS, "fido-u2f", kCommonName,
      },
  };

  virtual_device_.mutable_state()->attestation_cert_common_name = kCommonName;
  virtual_device_.mutable_state()->individual_attestation_cert_common_name =
      kCommonName;
  NavigateAndCommit(GURL("https://example.com"));

  RunTestCases(kTests);
}

TEST_F(AuthenticatorContentBrowserClientTest, Unfocused) {
  // When the |ContentBrowserClient| considers the tab to be unfocused,
  // registration requests should fail with a |NOT_FOCUSED| error, but getting
  // assertions should still work.
  test_client_.set_focused(false);

  NavigateAndCommit(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  {
    PublicKeyCredentialCreationOptionsPtr options =
        GetTestPublicKeyCredentialCreationOptions();
    options->public_key_parameters = GetTestPublicKeyCredentialParameters(123);

    TestMakeCredentialCallback cb;
    authenticator->MakeCredential(std::move(options), cb.callback());
    cb.WaitForCallback();
    EXPECT_EQ(AuthenticatorStatus::NOT_FOCUSED, cb.status());
  }

  {
    TestServiceManagerContext service_manager_context;

    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();

    auto credential = PublicKeyCredentialDescriptor::New();
    credential->type = PublicKeyCredentialType::PUBLIC_KEY;
    credential->id.resize(16);
    ASSERT_TRUE(virtual_device_.mutable_state()->InjectRegistration(
        credential->id, kTestRelyingPartyId));
    options->allow_credentials.emplace_back(std::move(credential));

    TestGetAssertionCallback cb;
    authenticator->GetAssertion(std::move(options), cb.callback());
    cb.WaitForCallback();

    EXPECT_EQ(AuthenticatorStatus::SUCCESS, cb.status());
  }
}

}  // namespace content
