// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A complete set of unit tests for GaiaAuthFetcher.
// Originally ported from GoogleAuthenticator tests.

#include <string>

#include "base/json/json_reader.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "build/build_config.h"
#include "google_apis/gaia/gaia_auth_consumer.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/mock_url_fetcher_factory.h"
#include "google_apis/google_api_keys.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::Invoke;
using ::testing::_;

namespace {

const char kGetAuthCodeValidCookie[] =
    "oauth_code=test-code; Path=/test; Secure; HttpOnly";
const char kGetAuthCodeCookieNoSecure[] =
    "oauth_code=test-code; Path=/test; HttpOnly";
const char kGetAuthCodeCookieNoHttpOnly[] =
    "oauth_code=test-code; Path=/test; Secure";
const char kGetAuthCodeCookieNoOAuthCode[] =
    "Path=/test; Secure; HttpOnly";
const char kGetTokenPairValidResponse[] =
    "{"
    "  \"refresh_token\": \"rt1\","
    "  \"access_token\": \"at1\","
    "  \"expires_in\": 3600,"
    "  \"token_type\": \"Bearer\","
    "  \"id_token\": \"it1\""
    "}";

}  // namespace

MockFetcher::MockFetcher(bool success,
                         const GURL& url,
                         const std::string& results,
                         net::URLFetcher::RequestType request_type,
                         net::URLFetcherDelegate* d)
    : TestURLFetcher(0, url, d) {
  set_url(url);
  net::Error error;

  if (success) {
    error = net::OK;
    set_response_code(net::HTTP_OK);
  } else {
    error = net::ERR_FAILED;
  }

  set_status(net::URLRequestStatus::FromError(error));
  SetResponseString(results);
}

MockFetcher::MockFetcher(const GURL& url,
                         const net::URLRequestStatus& status,
                         int response_code,
                         const std::string& results,
                         net::URLFetcher::RequestType request_type,
                         net::URLFetcherDelegate* d)
    : TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(status);
  set_response_code(response_code);
  SetResponseString(results);
}

MockFetcher::~MockFetcher() {}

void MockFetcher::Start() {
  delegate()->OnURLFetchComplete(this);
}

class GaiaAuthFetcherTest : public testing::Test {
 protected:
  GaiaAuthFetcherTest()
      : issue_auth_token_source_(
            GaiaUrls::GetInstance()->issue_auth_token_url()),
        client_login_to_oauth2_source_(
            GaiaUrls::GetInstance()->deprecated_client_login_to_oauth2_url()),
        oauth2_token_source_(GaiaUrls::GetInstance()->oauth2_token_url()),
        token_auth_source_(GaiaUrls::GetInstance()->token_auth_url()),
        merge_session_source_(GaiaUrls::GetInstance()->merge_session_url()),
        uberauth_token_source_(
            GaiaUrls::GetInstance()->oauth1_login_url().Resolve(
                "?source=&issueuberauth=1")),
        oauth_login_gurl_(GaiaUrls::GetInstance()->oauth1_login_url()) {}

  void RunParsingTest(const std::string& data,
                      const std::string& sid,
                      const std::string& lsid,
                      const std::string& token) {
    std::string out_sid;
    std::string out_lsid;
    std::string out_token;

    GaiaAuthFetcher::ParseClientLoginResponse(data,
                                              &out_sid,
                                              &out_lsid,
                                              &out_token);
    EXPECT_EQ(lsid, out_lsid);
    EXPECT_EQ(sid, out_sid);
    EXPECT_EQ(token, out_token);
  }

  void RunErrorParsingTest(const std::string& data,
                           const std::string& error,
                           const std::string& error_url,
                           const std::string& captcha_url,
                           const std::string& captcha_token) {
    std::string out_error;
    std::string out_error_url;
    std::string out_captcha_url;
    std::string out_captcha_token;

    GaiaAuthFetcher::ParseClientLoginFailure(data,
                                             &out_error,
                                             &out_error_url,
                                             &out_captcha_url,
                                             &out_captcha_token);
    EXPECT_EQ(error, out_error);
    EXPECT_EQ(error_url, out_error_url);
    EXPECT_EQ(captcha_url, out_captcha_url);
    EXPECT_EQ(captcha_token, out_captcha_token);
  }

  GURL issue_auth_token_source_;
  GURL client_login_to_oauth2_source_;
  GURL oauth2_token_source_;
  GURL token_auth_source_;
  GURL merge_session_source_;
  GURL uberauth_token_source_;
  GURL oauth_login_gurl_;

 protected:
  net::TestURLRequestContextGetter* GetRequestContext() {
    if (!request_context_getter_.get()) {
      request_context_getter_ = new net::TestURLRequestContextGetter(
          message_loop_.task_runner());
    }
    return request_context_getter_.get();
  }

  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
};

class MockGaiaConsumer : public GaiaAuthConsumer {
 public:
  MockGaiaConsumer() {}
  ~MockGaiaConsumer() override {}

  MOCK_METHOD1(OnClientLoginSuccess, void(const ClientLoginResult& result));
  MOCK_METHOD2(OnIssueAuthTokenSuccess, void(const std::string& service,
      const std::string& token));
  MOCK_METHOD1(OnClientOAuthCode, void(const std::string& data));
  MOCK_METHOD1(OnClientOAuthSuccess,
               void(const GaiaAuthConsumer::ClientOAuthResult& result));
  MOCK_METHOD1(OnMergeSessionSuccess, void(const std::string& data));
  MOCK_METHOD1(OnUberAuthTokenSuccess, void(const std::string& data));
  MOCK_METHOD1(OnClientLoginFailure,
      void(const GoogleServiceAuthError& error));
  MOCK_METHOD2(OnIssueAuthTokenFailure, void(const std::string& service,
      const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnClientOAuthFailure,
      void(const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnOAuth2RevokeTokenCompleted,
               void(GaiaAuthConsumer::TokenRevocationStatus status));
  MOCK_METHOD1(OnMergeSessionFailure, void(
      const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnUberAuthTokenFailure, void(
      const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnListAccountsSuccess, void(const std::string& data));
  MOCK_METHOD0(OnLogOutSuccess, void());
  MOCK_METHOD1(OnLogOutFailure, void(const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnGetCheckConnectionInfoSuccess, void(const std::string& data));
};

#if defined(OS_WIN)
#define MAYBE_ErrorComparator DISABLED_ErrorComparator
#else
#define MAYBE_ErrorComparator ErrorComparator
#endif

TEST_F(GaiaAuthFetcherTest, MAYBE_ErrorComparator) {
  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(-101);

  GoogleServiceAuthError matching_error =
      GoogleServiceAuthError::FromConnectionError(-101);

  EXPECT_TRUE(expected_error == matching_error);

  expected_error = GoogleServiceAuthError::FromConnectionError(6);

  EXPECT_FALSE(expected_error == matching_error);

  expected_error = GoogleServiceAuthError(GoogleServiceAuthError::NONE);

  EXPECT_FALSE(expected_error == matching_error);

  matching_error = GoogleServiceAuthError(GoogleServiceAuthError::NONE);

  EXPECT_TRUE(expected_error == matching_error);
}

TEST_F(GaiaAuthFetcherTest, TokenNetFailure) {
  int error_no = net::ERR_CONNECTION_RESET;
  net::URLRequestStatus status(net::URLRequestStatus::FAILED, error_no);

  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(error_no);

  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnIssueAuthTokenFailure(_, expected_error))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());

  MockFetcher mock_fetcher(issue_auth_token_source_, status, 0, std::string(),
                           net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}


TEST_F(GaiaAuthFetcherTest, ParseRequest) {
  RunParsingTest("SID=sid\nLSID=lsid\nAuth=auth\n", "sid", "lsid", "auth");
  RunParsingTest("LSID=lsid\nSID=sid\nAuth=auth\n", "sid", "lsid", "auth");
  RunParsingTest("SID=sid\nLSID=lsid\nAuth=auth", "sid", "lsid", "auth");
  RunParsingTest("SID=sid\nAuth=auth\n", "sid", std::string(), "auth");
  RunParsingTest("LSID=lsid\nAuth=auth\n", std::string(), "lsid", "auth");
  RunParsingTest("\nAuth=auth\n", std::string(), std::string(), "auth");
  RunParsingTest("SID=sid", "sid", std::string(), std::string());
}

TEST_F(GaiaAuthFetcherTest, ParseErrorRequest) {
  RunErrorParsingTest("Url=U\n"
                      "Error=E\n"
                      "CaptchaToken=T\n"
                      "CaptchaUrl=C\n", "E", "U", "C", "T");
  RunErrorParsingTest("CaptchaToken=T\n"
                      "Error=E\n"
                      "Url=U\n"
                      "CaptchaUrl=C\n", "E", "U", "C", "T");
  RunErrorParsingTest("\n\n\nCaptchaToken=T\n"
                      "\nError=E\n"
                      "\nUrl=U\n"
                      "CaptchaUrl=C\n", "E", "U", "C", "T");
}

TEST_F(GaiaAuthFetcherTest, WorkingIssueAuthToken) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnIssueAuthTokenSuccess(_, "token"))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(issue_auth_token_source_, status, net::HTTP_OK,
                           "token", net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, CheckTwoFactorResponse) {
  std::string response =
      base::StringPrintf("Error=BadAuthentication\n%s\n",
                         GaiaAuthFetcher::kSecondFactor);
  EXPECT_TRUE(GaiaAuthFetcher::IsSecondFactorSuccess(response));
}

TEST_F(GaiaAuthFetcherTest, CheckNormalErrorCode) {
  std::string response = "Error=BadAuthentication\n";
  EXPECT_FALSE(GaiaAuthFetcher::IsSecondFactorSuccess(response));
}

TEST_F(GaiaAuthFetcherTest, CaptchaParse) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Url=http://www.google.com/login/captcha\n"
                     "Error=CaptchaRequired\n"
                     "CaptchaToken=CCTOKEN\n"
                     "CaptchaUrl=Captcha?ctoken=CCTOKEN\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);

  std::string token = "CCTOKEN";
  GURL image_url("http://accounts.google.com/Captcha?ctoken=CCTOKEN");
  GURL unlock_url("http://www.google.com/login/captcha");

  EXPECT_EQ(error.state(), GoogleServiceAuthError::CAPTCHA_REQUIRED);
  EXPECT_EQ(error.captcha().token, token);
  EXPECT_EQ(error.captcha().image_url, image_url);
  EXPECT_EQ(error.captcha().unlock_url, unlock_url);
}

TEST_F(GaiaAuthFetcherTest, AccountDeletedError) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Error=AccountDeleted\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);
  EXPECT_EQ(error.state(), GoogleServiceAuthError::ACCOUNT_DELETED);
}

TEST_F(GaiaAuthFetcherTest, AccountDisabledError) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Error=AccountDisabled\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);
  EXPECT_EQ(error.state(), GoogleServiceAuthError::ACCOUNT_DISABLED);
}

TEST_F(GaiaAuthFetcherTest, BadAuthenticationError) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Error=BadAuthentication\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);
  EXPECT_EQ(error.state(), GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
}

TEST_F(GaiaAuthFetcherTest, IncomprehensibleError) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Error=Gobbledygook\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);
  EXPECT_EQ(error.state(), GoogleServiceAuthError::SERVICE_UNAVAILABLE);
}

TEST_F(GaiaAuthFetcherTest, ServiceUnavailableError) {
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  std::string data = "Error=ServiceUnavailable\n";
  GoogleServiceAuthError error =
      GaiaAuthFetcher::GenerateAuthError(data, status);
  EXPECT_EQ(error.state(), GoogleServiceAuthError::SERVICE_UNAVAILABLE);
}

TEST_F(GaiaAuthFetcherTest, OAuthLoginTokenSuccess) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnClientOAuthCode("test-code")).Times(0);
  EXPECT_CALL(consumer,
              OnClientOAuthSuccess(GaiaAuthConsumer::ClientOAuthResult(
                  "rt1", "at1", 3600, false /* is_child_account */)))
      .Times(1);

  net::TestURLFetcherFactory factory;
  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.DeprecatedStartCookieForOAuthLoginTokenExchange("0");
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  EXPECT_TRUE(NULL != fetcher);
  EXPECT_EQ(net::LOAD_NORMAL, fetcher->GetLoadFlags());
  EXPECT_EQ(std::string::npos,
            fetcher->GetOriginalURL().query().find("device_type=chrome"));

  EXPECT_TRUE(auth.HasPendingFetch());
  scoped_refptr<net::HttpResponseHeaders> reponse_headers =
      new net::HttpResponseHeaders("");
  reponse_headers->AddCookie(kGetAuthCodeValidCookie);
  MockFetcher mock_fetcher1(
      client_login_to_oauth2_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0),
      net::HTTP_OK,
      std::string(),
      net::URLFetcher::POST,
      &auth);
  mock_fetcher1.set_response_headers(reponse_headers);
  auth.OnURLFetchComplete(&mock_fetcher1);
  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher2(
      oauth2_token_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0), net::HTTP_OK,
      kGetTokenPairValidResponse, net::URLFetcher::POST, &auth);
  auth.OnURLFetchComplete(&mock_fetcher2);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, OAuthLoginTokenSuccessNoTokenFetch) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnClientOAuthCode("test-code")).Times(1);
  EXPECT_CALL(consumer,
              OnClientOAuthSuccess(GaiaAuthConsumer::ClientOAuthResult(
                  "", "", 0, false /* is_child_account */)))
      .Times(0);

  net::TestURLFetcherFactory factory;
  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.DeprecatedStartCookieForOAuthLoginTokenExchange(false, "0",
                                                       "ABCDE_12345", "");
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  EXPECT_TRUE(NULL != fetcher);
  EXPECT_EQ(net::LOAD_NORMAL, fetcher->GetLoadFlags());
  EXPECT_EQ(std::string::npos,
            fetcher->GetOriginalURL().query().find("device_type=chrome"));

  scoped_refptr<net::HttpResponseHeaders> reponse_headers =
      new net::HttpResponseHeaders("");
  reponse_headers->AddCookie(kGetAuthCodeValidCookie);
  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher1(
      client_login_to_oauth2_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0),
      net::HTTP_OK,
      std::string(),
      net::URLFetcher::POST,
      &auth);
  mock_fetcher1.set_response_headers(reponse_headers);
  auth.OnURLFetchComplete(&mock_fetcher1);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, OAuthLoginTokenWithCookies_DeviceId) {
  MockGaiaConsumer consumer;
  net::TestURLFetcherFactory factory;
  std::string expected_device_id("ABCDE-12345");
  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.DeprecatedStartCookieForOAuthLoginTokenExchangeWithDeviceId(
      "0", expected_device_id);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  EXPECT_TRUE(NULL != fetcher);
  EXPECT_EQ(net::LOAD_NORMAL, fetcher->GetLoadFlags());
  EXPECT_NE(std::string::npos,
            fetcher->GetOriginalURL().query().find("device_type=chrome"));
  net::HttpRequestHeaders extra_request_headers;
  fetcher->GetExtraRequestHeaders(&extra_request_headers);
  std::string device_id;
  EXPECT_TRUE(extra_request_headers.GetHeader("X-Device-ID", &device_id));
  EXPECT_EQ(device_id, expected_device_id);
}

TEST_F(GaiaAuthFetcherTest, OAuthLoginTokenClientLoginToOAuth2Failure) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnClientOAuthFailure(_))
      .Times(1);

  net::TestURLFetcherFactory factory;
  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.DeprecatedStartCookieForOAuthLoginTokenExchange(std::string());

  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher(
      client_login_to_oauth2_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0),
      net::HTTP_FORBIDDEN,
      std::string(),
      net::URLFetcher::POST,
      &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, OAuthLoginTokenOAuth2TokenPairFailure) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnClientOAuthFailure(_))
      .Times(1);

  net::TestURLFetcherFactory factory;
  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.DeprecatedStartCookieForOAuthLoginTokenExchange(std::string());

  scoped_refptr<net::HttpResponseHeaders> reponse_headers =
      new net::HttpResponseHeaders("");
  reponse_headers->AddCookie(kGetAuthCodeValidCookie);
  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher1(
      client_login_to_oauth2_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0),
      net::HTTP_OK,
      std::string(),
      net::URLFetcher::POST,
      &auth);
  mock_fetcher1.set_response_headers(reponse_headers);
  auth.OnURLFetchComplete(&mock_fetcher1);
  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher2(
      oauth2_token_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0),
      net::HTTP_FORBIDDEN,
      std::string(),
      net::URLFetcher::POST,
      &auth);
  auth.OnURLFetchComplete(&mock_fetcher2);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, MergeSessionSuccess) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnMergeSessionSuccess("<html></html>"))
      .Times(1);

  net::TestURLFetcherFactory factory;

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.StartMergeSession("myubertoken", std::string());

  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher(
      merge_session_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0), net::HTTP_OK,
      "<html></html>", net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, MergeSessionSuccessRedirect) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnMergeSessionSuccess("<html></html>"))
      .Times(1);

  net::TestURLFetcherFactory factory;

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.StartMergeSession("myubertoken", std::string());

  // Make sure the fetcher created has the expected flags.  Set its url()
  // properties to reflect a redirect.
  net::TestURLFetcher* test_fetcher = factory.GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher != NULL);
  EXPECT_TRUE(test_fetcher->GetLoadFlags() == net::LOAD_NORMAL);
  EXPECT_TRUE(auth.HasPendingFetch());

  GURL final_url("http://www.google.com/CheckCookie");
  test_fetcher->set_url(final_url);
  test_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  test_fetcher->set_response_code(net::HTTP_OK);
  test_fetcher->SetResponseString("<html></html>");

  auth.OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, UberAuthTokenSuccess) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnUberAuthTokenSuccess("uberToken"))
      .Times(1);

  net::TestURLFetcherFactory factory;

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  auth.StartTokenFetchForUberAuthExchange("myAccessToken",
                                          true /* is_bound_to_channel_id */);

  EXPECT_TRUE(auth.HasPendingFetch());
  MockFetcher mock_fetcher(
      uberauth_token_source_,
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0), net::HTTP_OK,
      "uberToken", net::URLFetcher::POST, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
  EXPECT_FALSE(auth.HasPendingFetch());
}

TEST_F(GaiaAuthFetcherTest, ParseClientLoginToOAuth2Response) {
  {  // No cookies.
    std::string auth_code;
    net::ResponseCookies cookies;
    EXPECT_FALSE(GaiaAuthFetcher::ParseClientLoginToOAuth2Response(
        cookies, &auth_code));
    EXPECT_EQ("", auth_code);
  }
  {  // Few cookies, nothing appropriate.
    std::string auth_code;
    net::ResponseCookies cookies;
    cookies.push_back(kGetAuthCodeCookieNoSecure);
    cookies.push_back(kGetAuthCodeCookieNoHttpOnly);
    cookies.push_back(kGetAuthCodeCookieNoOAuthCode);
    EXPECT_FALSE(GaiaAuthFetcher::ParseClientLoginToOAuth2Response(
        cookies, &auth_code));
    EXPECT_EQ("", auth_code);
  }
  {  // Few cookies, one of them is valid.
    std::string auth_code;
    net::ResponseCookies cookies;
    cookies.push_back(kGetAuthCodeCookieNoSecure);
    cookies.push_back(kGetAuthCodeCookieNoHttpOnly);
    cookies.push_back(kGetAuthCodeCookieNoOAuthCode);
    cookies.push_back(kGetAuthCodeValidCookie);
    EXPECT_TRUE(GaiaAuthFetcher::ParseClientLoginToOAuth2Response(
        cookies, &auth_code));
    EXPECT_EQ("test-code", auth_code);
  }
  {  // Single valid cookie (like in real responses).
    std::string auth_code;
    net::ResponseCookies cookies;
    cookies.push_back(kGetAuthCodeValidCookie);
    EXPECT_TRUE(GaiaAuthFetcher::ParseClientLoginToOAuth2Response(
        cookies, &auth_code));
    EXPECT_EQ("test-code", auth_code);
  }
}

TEST_F(GaiaAuthFetcherTest, StartOAuthLogin) {
  // OAuthLogin returns the same as the ClientLogin endpoint, minus CAPTCHA
  // responses.
  std::string data("SID=sid\nLSID=lsid\nAuth=auth\n");

  GaiaAuthConsumer::ClientLoginResult result;
  result.lsid = "lsid";
  result.sid = "sid";
  result.token = "auth";
  result.data = data;

  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnClientLoginSuccess(result))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(oauth_login_gurl_, status, net::HTTP_OK, data,
                           net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, ListAccounts) {
  std::string data("[\"gaia.l.a.r\", ["
      "[\"gaia.l.a\", 1, \"First Last\", \"user@gmail.com\", "
      "\"//googleusercontent.com/A/B/C/D/photo.jpg\", 1, 1, 0]]]");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnListAccountsSuccess(data)).Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(
      GaiaUrls::GetInstance()->ListAccountsURLWithSource(std::string()), status,
      net::HTTP_OK, data, net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, LogOutSuccess) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnLogOutSuccess()).Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(
      GaiaUrls::GetInstance()->LogOutURLWithSource(std::string()), status,
      net::HTTP_OK, std::string(), net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, LogOutFailure) {
  int error_no = net::ERR_CONNECTION_RESET;
  net::URLRequestStatus status(net::URLRequestStatus::FAILED, error_no);

  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(error_no);
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnLogOutFailure(expected_error)).Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());

  MockFetcher mock_fetcher(
      GaiaUrls::GetInstance()->LogOutURLWithSource(std::string()), status, 0,
      std::string(), net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, GetCheckConnectionInfo) {
  std::string data(
      "[{\"carryBackToken\": \"token1\", \"url\": \"http://www.google.com\"}]");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnGetCheckConnectionInfoSuccess(data)).Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(
      GaiaUrls::GetInstance()->GetCheckConnectionInfoURLWithSource(
          std::string()),
      status, net::HTTP_OK, data, net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenSuccess) {
  std::string data("{}");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer, OnOAuth2RevokeTokenCompleted(
                            GaiaAuthConsumer::TokenRevocationStatus::kSuccess))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           net::HTTP_OK, data, net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenCanceled) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kConnectionCanceled))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::CANCELED,
                               net::ERR_ABORTED);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           0, std::string(), net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenFailed) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kConnectionFailed))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  int error_no = net::ERR_CERT_CONTAINS_ERRORS;
  net::URLRequestStatus status(net::URLRequestStatus::FAILED, error_no);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           0, std::string(), net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenTimeout) {
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kConnectionTimeout))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  int error_no = net::ERR_TIMED_OUT;
  net::URLRequestStatus status(net::URLRequestStatus::FAILED, error_no);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           0, std::string(), net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenInvalidToken) {
  std::string data("{\"error\" : \"invalid_token\"}");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kInvalidToken))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           net::HTTP_BAD_REQUEST, data, net::URLFetcher::GET,
                           &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenInvalidRequest) {
  std::string data("{\"error\" : \"invalid_request\"}");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kInvalidRequest))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           net::HTTP_BAD_REQUEST, data, net::URLFetcher::GET,
                           &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}

TEST_F(GaiaAuthFetcherTest, RevokeOAuth2TokenServerError) {
  std::string data("{}");
  MockGaiaConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuth2RevokeTokenCompleted(
                  GaiaAuthConsumer::TokenRevocationStatus::kServerError))
      .Times(1);

  GaiaAuthFetcher auth(&consumer, std::string(), GetRequestContext());
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  MockFetcher mock_fetcher(GaiaUrls::GetInstance()->oauth2_revoke_url(), status,
                           net::HTTP_INTERNAL_SERVER_ERROR, data,
                           net::URLFetcher::GET, &auth);
  auth.OnURLFetchComplete(&mock_fetcher);
}
