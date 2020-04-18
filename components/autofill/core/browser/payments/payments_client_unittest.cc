// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card_save_manager.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_http_header_provider.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "services/identity/public/cpp/identity_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace payments {
namespace {

int kAllDetectableValues =
    CreditCardSaveManager::DetectedValue::CVC |
    CreditCardSaveManager::DetectedValue::CARDHOLDER_NAME |
    CreditCardSaveManager::DetectedValue::ADDRESS_NAME |
    CreditCardSaveManager::DetectedValue::ADDRESS_LINE |
    CreditCardSaveManager::DetectedValue::LOCALITY |
    CreditCardSaveManager::DetectedValue::ADMINISTRATIVE_AREA |
    CreditCardSaveManager::DetectedValue::POSTAL_CODE |
    CreditCardSaveManager::DetectedValue::COUNTRY_CODE |
    CreditCardSaveManager::DetectedValue::HAS_GOOGLE_PAYMENTS_ACCOUNT;

}  // namespace

class PaymentsClientTest : public testing::Test,
                           public PaymentsClientUnmaskDelegate,
                           public PaymentsClientSaveDelegate {
 public:
  PaymentsClientTest() : result_(AutofillClient::NONE) {}
  ~PaymentsClientTest() override {}

  void SetUp() override {
    // Silence the warning for mismatching sync and Payments servers.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kWalletServiceUseSandbox, "0");

    result_ = AutofillClient::NONE;
    server_id_.clear();
    real_pan_.clear();
    legal_message_.reset();

    request_context_ = new net::TestURLRequestContextGetter(
        base::ThreadTaskRunnerHandle::Get());
    TestingPrefServiceSimple pref_service_;
    client_.reset(new PaymentsClient(request_context_.get(), &pref_service_,
                                     identity_test_env_.identity_manager(),
                                     this, this));
  }

  void TearDown() override { client_.reset(); }

  void DisableAutofillUpstreamSendDetectedValuesExperiment() {
    scoped_feature_list_.InitAndDisableFeature(
        kAutofillUpstreamSendDetectedValues);
  }

  void EnableAutofillUpstreamSendDetectedValuesExperiment() {
    scoped_feature_list_.InitAndEnableFeature(
        kAutofillUpstreamSendDetectedValues);
  }

  void EnableAutofillUpstreamSendPanFirstSixExperiment() {
    scoped_feature_list_.InitAndEnableFeature(kAutofillUpstreamSendPanFirstSix);
  }

  void DisableAutofillSendExperimentIdsInPaymentsRPCs() {
    scoped_feature_list_.InitAndDisableFeature(
        features::kAutofillSendExperimentIdsInPaymentsRPCs);
  }

  // Registers a field trial with the specified name and group and an associated
  // google web property variation id.
  void CreateFieldTrialWithId(const std::string& trial_name,
                              const std::string& group_name,
                              int variation_id) {
    variations::AssociateGoogleVariationID(
        variations::GOOGLE_WEB_PROPERTIES, trial_name, group_name,
        static_cast<variations::VariationID>(variation_id));
    base::FieldTrialList::CreateFieldTrial(trial_name, group_name)->group();
  }

  // PaymentsClientUnmaskDelegate:
  void OnDidGetRealPan(AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan) override {
    result_ = result;
    real_pan_ = real_pan;
  }

  // PaymentsClientSaveDelegate:
  void OnDidGetUploadDetails(
      AutofillClient::PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message) override {
    result_ = result;
    legal_message_ = std::move(legal_message);
  }
  void OnDidUploadCard(AutofillClient::PaymentsRpcResult result,
                       const std::string& server_id) override {
    result_ = result;
    server_id_ = server_id;
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;

  void StartUnmasking() {
    if (!identity_test_env_.identity_manager()->HasPrimaryAccount())
      identity_test_env_.MakePrimaryAccountAvailable("example@gmail.com");

    PaymentsClient::UnmaskRequestDetails request_details;
    request_details.billing_customer_number = 111222333444;
    request_details.card = test::GetMaskedServerCard();
    request_details.user_response.cvc = base::ASCIIToUTF16("123");
    request_details.risk_data = "some risk data";
    client_->UnmaskCard(request_details);
  }

  void StartGettingUploadDetails() {
    if (!identity_test_env_.identity_manager()->HasPrimaryAccount())
      identity_test_env_.MakePrimaryAccountAvailable("example@gmail.com");

    client_->GetUploadDetails(BuildTestProfiles(), kAllDetectableValues,
                              /*pan_first_six=*/"411111",
                              std::vector<const char*>(), "language-LOCALE");
  }

  void StartUploading(bool include_cvc) {
    if (!identity_test_env_.identity_manager()->HasPrimaryAccount())
      identity_test_env_.MakePrimaryAccountAvailable("example@gmail.com");

    PaymentsClient::UploadRequestDetails request_details;
    request_details.billing_customer_number = 111222333444;
    request_details.card = test::GetCreditCard();
    if (include_cvc)
      request_details.cvc = base::ASCIIToUTF16("123");
    request_details.context_token = base::ASCIIToUTF16("context token");
    request_details.risk_data = "some risk data";
    request_details.app_locale = "language-LOCALE";
    request_details.profiles = BuildTestProfiles();
    client_->UploadCard(request_details);
  }

  const std::string& GetUploadData() {
    return factory_.GetFetcherByID(0)->upload_data();
  }

  void IssueOAuthToken() {
    identity_test_env_.WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
        "totally_real_token",
        base::Time::Now() + base::TimeDelta::FromDays(10));

    // Verify the auth header.
    net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
    net::HttpRequestHeaders request_headers;
    fetcher->GetExtraRequestHeaders(&request_headers);
    std::string auth_header_value;
    EXPECT_TRUE(request_headers.GetHeader(
        net::HttpRequestHeaders::kAuthorization, &auth_header_value))
        << request_headers.ToString();
    EXPECT_EQ("Bearer totally_real_token", auth_header_value);
  }

  void ReturnResponse(net::HttpStatusCode response_code,
                      const std::string& response_body) {
    net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);
    fetcher->set_response_code(response_code);
    fetcher->SetResponseString(response_body);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  AutofillClient::PaymentsRpcResult result_;
  std::string server_id_;
  std::string real_pan_;
  std::unique_ptr<base::DictionaryValue> legal_message_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::TestURLFetcherFactory factory_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  std::unique_ptr<PaymentsClient> client_;
  identity::IdentityTestEnvironment identity_test_env_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentsClientTest);

  std::vector<AutofillProfile> BuildTestProfiles() {
    std::vector<AutofillProfile> profiles;
    profiles.push_back(BuildProfile("John", "Smith", "1234 Main St.", "Miami",
                                    "FL", "32006", "212-555-0162"));
    profiles.push_back(BuildProfile("Pat", "Jones", "432 Oak Lane", "Lincoln",
                                    "OH", "43005", "(834)555-0090"));
    return profiles;
  }

  AutofillProfile BuildProfile(base::StringPiece first_name,
                               base::StringPiece last_name,
                               base::StringPiece address_line,
                               base::StringPiece city,
                               base::StringPiece state,
                               base::StringPiece zip,
                               base::StringPiece phone_number) {
    AutofillProfile profile;

    profile.SetInfo(NAME_FIRST, ASCIIToUTF16(first_name), "en-US");
    profile.SetInfo(NAME_LAST, ASCIIToUTF16(last_name), "en-US");
    profile.SetInfo(ADDRESS_HOME_LINE1, ASCIIToUTF16(address_line), "en-US");
    profile.SetInfo(ADDRESS_HOME_CITY, ASCIIToUTF16(city), "en-US");
    profile.SetInfo(ADDRESS_HOME_STATE, ASCIIToUTF16(state), "en-US");
    profile.SetInfo(ADDRESS_HOME_ZIP, ASCIIToUTF16(zip), "en-US");
    profile.SetInfo(PHONE_HOME_WHOLE_NUMBER, ASCIIToUTF16(phone_number),
                    "en-US");

    return profile;
  }
};

TEST_F(PaymentsClientTest, OAuthError) {
  StartUnmasking();
  identity_test_env_.WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
  EXPECT_TRUE(real_pan_.empty());
}

TEST_F(PaymentsClientTest,
       UnmaskRequestIncludesBillingCustomerNumberInRequest) {
  StartUnmasking();

  // Verify that the billing customer number is included in the request.
  EXPECT_TRUE(
      GetUploadData().find("%22external_customer_id%22:%22111222333444%22") !=
      std::string::npos);
}

TEST_F(PaymentsClientTest, UnmaskSuccess) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{ \"pan\": \"1234\" }");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);
  EXPECT_EQ("1234", real_pan_);
}

TEST_F(PaymentsClientTest, GetDetailsSuccess) {
  StartGettingUploadDetails();
  ReturnResponse(
      net::HTTP_OK,
      "{ \"context_token\": \"some_token\", \"legal_message\": {} }");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);
  EXPECT_NE(nullptr, legal_message_.get());
}

TEST_F(PaymentsClientTest, GetDetailsRemovesNonLocationData) {
  StartGettingUploadDetails();

  // Verify that the recipient name field and test names appear nowhere in the
  // upload data.
  EXPECT_TRUE(GetUploadData().find(PaymentsClient::kRecipientName) ==
              std::string::npos);
  EXPECT_TRUE(GetUploadData().find("John") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Smith") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Pat") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Jones") == std::string::npos);

  // Verify that the phone number field and test numbers appear nowhere in the
  // upload data.
  EXPECT_TRUE(GetUploadData().find(PaymentsClient::kPhoneNumber) ==
              std::string::npos);
  EXPECT_TRUE(GetUploadData().find("212") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("555") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("0162") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("834") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("0090") == std::string::npos);
}

TEST_F(PaymentsClientTest,
       GetDetailsIncludesDetectedValuesInRequestIfExperimentOn) {
  EnableAutofillUpstreamSendDetectedValuesExperiment();

  StartGettingUploadDetails();

  // Verify that the detected values were included in the request.
  std::string detected_values_string =
      "\"detected_values\":" + std::to_string(kAllDetectableValues);
  EXPECT_TRUE(GetUploadData().find(detected_values_string) !=
              std::string::npos);
}

TEST_F(PaymentsClientTest,
       GetDetailsDoesNotIncludeDetectedValuesInRequestIfExperimentOff) {
  DisableAutofillUpstreamSendDetectedValuesExperiment();

  StartGettingUploadDetails();

  // Verify that the detected values were left out of the request.
  std::string detected_values_string =
      "\"detected_values\":" + std::to_string(kAllDetectableValues);
  EXPECT_TRUE(GetUploadData().find(detected_values_string) ==
              std::string::npos);
}

TEST_F(PaymentsClientTest, GetUploadDetailsVariationsTest) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartGettingUploadDetails();

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_FALSE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest, GetUploadDetailsVariationsTestExperimentFlagOff) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  DisableAutofillSendExperimentIdsInPaymentsRPCs();
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartGettingUploadDetails();

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_FALSE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_TRUE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest, UploadCardVariationsTest) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartUploading(/*include_cvc=*/true);

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_FALSE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest, UploadCardVariationsTestExperimentFlagOff) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  DisableAutofillSendExperimentIdsInPaymentsRPCs();
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartUploading(/*include_cvc=*/true);

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_FALSE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_TRUE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest, UnmaskCardVariationsTest) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartUnmasking();

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_FALSE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest, UnmaskCardVariationsTestExperimentOff) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  DisableAutofillSendExperimentIdsInPaymentsRPCs();
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("AutofillTest", "Group", 369);
  StartUnmasking();

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_FALSE(headers.GetHeader("X-Client-Data", &value));
  // Note that experiment information is stored in X-Client-Data.
  EXPECT_TRUE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(PaymentsClientTest,
       GetDetailsIncludesPanFirstSixInRequestIfExperimentOn) {
  EnableAutofillUpstreamSendPanFirstSixExperiment();

  StartGettingUploadDetails();

  // Verify that the value of pan_first_six was included in the request.
  EXPECT_TRUE(GetUploadData().find("\"pan_first6\":\"411111\"") !=
              std::string::npos);
}

TEST_F(PaymentsClientTest,
       GetDetailsDoesNotIncludePanFirstSixInRequestIfExperimentOff) {
  StartGettingUploadDetails();

  // Verify that the value of pan_first_six was left out of the request.
  EXPECT_TRUE(GetUploadData().find("\"pan_first6\":\"411111\"") ==
              std::string::npos);
}

TEST_F(PaymentsClientTest, UploadSuccessWithoutServerId) {
  StartUploading(/*include_cvc=*/true);
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{}");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);
  EXPECT_TRUE(server_id_.empty());
}

TEST_F(PaymentsClientTest, UploadSuccessWithServerId) {
  StartUploading(/*include_cvc=*/true);
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{ \"credit_card_id\": \"InstrumentData:1\" }");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);
  EXPECT_EQ("InstrumentData:1", server_id_);
}

TEST_F(PaymentsClientTest, UploadIncludesNonLocationData) {
  StartUploading(/*include_cvc=*/true);

  // Verify that the recipient name field and test names do appear in the upload
  // data.
  EXPECT_TRUE(GetUploadData().find(PaymentsClient::kRecipientName) !=
              std::string::npos);
  EXPECT_TRUE(GetUploadData().find("John") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Smith") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Pat") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("Jones") != std::string::npos);

  // Verify that the phone number field and test numbers do appear in the upload
  // data.
  EXPECT_TRUE(GetUploadData().find(PaymentsClient::kPhoneNumber) !=
              std::string::npos);
  EXPECT_TRUE(GetUploadData().find("212") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("555") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("0162") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("834") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("0090") != std::string::npos);
}

TEST_F(PaymentsClientTest,
       UploadRequestIncludesBillingCustomerNumberInRequest) {
  StartUploading(/*include_cvc=*/true);

  // Verify that the billing customer number is included in the request.
  EXPECT_TRUE(
      GetUploadData().find("%22external_customer_id%22:%22111222333444%22") !=
      std::string::npos);
}

TEST_F(PaymentsClientTest, UploadIncludesCvcInRequestIfProvided) {
  StartUploading(/*include_cvc=*/true);

  // Verify that the encrypted_cvc and s7e_13_cvc parameters were included in
  // the request.
  EXPECT_TRUE(GetUploadData().find("encrypted_cvc") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("__param:s7e_13_cvc") != std::string::npos);
  EXPECT_TRUE(GetUploadData().find("&s7e_13_cvc=") != std::string::npos);
}

TEST_F(PaymentsClientTest, UploadDoesNotIncludeCvcInRequestIfNotProvided) {
  StartUploading(/*include_cvc=*/false);

  // Verify that the encrypted_cvc and s7e_13_cvc parameters were not included
  // in the request.
  EXPECT_TRUE(GetUploadData().find("encrypted_cvc") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("__param:s7e_13_cvc") == std::string::npos);
  EXPECT_TRUE(GetUploadData().find("&s7e_13_cvc=") == std::string::npos);
}

TEST_F(PaymentsClientTest, GetDetailsFollowedByUploadSuccess) {
  StartGettingUploadDetails();
  ReturnResponse(
      net::HTTP_OK,
      "{ \"context_token\": \"some_token\", \"legal_message\": {} }");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);

  result_ = AutofillClient::NONE;

  StartUploading(/*include_cvc=*/true);
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{}");
  EXPECT_EQ(AutofillClient::SUCCESS, result_);
}

TEST_F(PaymentsClientTest, UnmaskMissingPan) {
  StartUnmasking();
  ReturnResponse(net::HTTP_OK, "{}");
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
}

TEST_F(PaymentsClientTest, GetDetailsMissingContextToken) {
  StartGettingUploadDetails();
  ReturnResponse(net::HTTP_OK, "{ \"legal_message\": {} }");
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
}

TEST_F(PaymentsClientTest, GetDetailsMissingLegalMessage) {
  StartGettingUploadDetails();
  ReturnResponse(net::HTTP_OK, "{ \"context_token\": \"some_token\" }");
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
  EXPECT_EQ(nullptr, legal_message_.get());
}

TEST_F(PaymentsClientTest, RetryFailure) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{ \"error\": { \"code\": \"INTERNAL\" } }");
  EXPECT_EQ(AutofillClient::TRY_AGAIN_FAILURE, result_);
  EXPECT_EQ("", real_pan_);
}

TEST_F(PaymentsClientTest, PermanentFailure) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK,
                 "{ \"error\": { \"code\": \"ANYTHING_ELSE\" } }");
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
  EXPECT_EQ("", real_pan_);
}

TEST_F(PaymentsClientTest, MalformedResponse) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_OK, "{ \"error_code\": \"WRONG_JSON_FORMAT\" }");
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
  EXPECT_EQ("", real_pan_);
}

TEST_F(PaymentsClientTest, ReauthNeeded) {
  {
    StartUnmasking();
    IssueOAuthToken();
    ReturnResponse(net::HTTP_UNAUTHORIZED, "");
    // No response yet.
    EXPECT_EQ(AutofillClient::NONE, result_);
    EXPECT_EQ("", real_pan_);

    // Second HTTP_UNAUTHORIZED causes permanent failure.
    IssueOAuthToken();
    ReturnResponse(net::HTTP_UNAUTHORIZED, "");
    EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
    EXPECT_EQ("", real_pan_);
  }

  result_ = AutofillClient::NONE;
  real_pan_.clear();

  {
    StartUnmasking();
    // NOTE: Don't issue an access token here: the issuing of an access token
    // first waits for the access token request to be received, but here there
    // should be no access token request because PaymentsClient should reuse the
    // access token from the previous request.
    ReturnResponse(net::HTTP_UNAUTHORIZED, "");
    // No response yet.
    EXPECT_EQ(AutofillClient::NONE, result_);
    EXPECT_EQ("", real_pan_);

    // HTTP_OK after first HTTP_UNAUTHORIZED results in success.
    IssueOAuthToken();
    ReturnResponse(net::HTTP_OK, "{ \"pan\": \"1234\" }");
    EXPECT_EQ(AutofillClient::SUCCESS, result_);
    EXPECT_EQ("1234", real_pan_);
  }
}

TEST_F(PaymentsClientTest, NetworkError) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_REQUEST_TIMEOUT, std::string());
  EXPECT_EQ(AutofillClient::NETWORK_ERROR, result_);
  EXPECT_EQ("", real_pan_);
}

TEST_F(PaymentsClientTest, OtherError) {
  StartUnmasking();
  IssueOAuthToken();
  ReturnResponse(net::HTTP_FORBIDDEN, std::string());
  EXPECT_EQ(AutofillClient::PERMANENT_FAILURE, result_);
  EXPECT_EQ("", real_pan_);
}

}  // namespace payments
}  // namespace autofill
