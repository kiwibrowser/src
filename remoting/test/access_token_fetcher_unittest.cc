// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/access_token_fetcher.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kAuthCodeValue[] = "test_auth_code_value";
const char kAccessTokenValue[] = "test_access_token_value";
const char kRefreshTokenValue[] = "test_refresh_token_value";
const char kAuthCodeExchangeValidResponse[] =
    "{"
    "  \"refresh_token\": \"test_refresh_token_value\","
    "  \"access_token\": \"test_access_token_value\","
    "  \"expires_in\": 3600,"
    "  \"token_type\": \"Bearer\""
    "}";
const char kAuthCodeExchangeEmptyResponse[] = "{}";
const char kRefreshTokenExchangeValidResponse[] =
    "{"
    "  \"access_token\": \"test_access_token_value\","
    "  \"expires_in\": 3600,"
    "  \"token_type\": \"Bearer\""
    "}";
const char kRefreshTokenExchangeEmptyResponse[] = "{}";
const char kValidTokenInfoResponse[] =
    "{"
    "  \"audience\": \"blah.apps.googleusercontent.blah.com\","
    "  \"used_id\": \"1234567890\","
    "  \"scope\": \"all the things\","
    "  \"expires_in\": \"1800\","
    "  \"token_type\": \"Bearer\""
    "}";
const char kInvalidTokenInfoResponse[] =
    "{"
    "  \"error\": \"invalid_token\""
    "}";
}  // namespace

namespace remoting {
namespace test {

// Provides base functionality for the AccessTokenFetcher Tests below.  The
// FakeURLFetcherFactory allows us to override the response data and payload for
// specified URLs.  We use this to stub out network calls made by the
// AccessTokenFetcher.  This fixture also creates an IO MessageLoop, if
// necessary, for use by the AccessTokenFetcher.
class AccessTokenFetcherTest : public ::testing::Test {
 public:
  AccessTokenFetcherTest();
  ~AccessTokenFetcherTest() override;

  void OnAccessTokenRetrieved(base::Closure done_closure,
                              const std::string& access_token,
                              const std::string& refresh_token);

 protected:
  // Test interface.
  void SetUp() override;

  void SetFakeResponse(const GURL& url,
                       const std::string& data,
                       net::HttpStatusCode code,
                       net::URLRequestStatus::Status status);

  // Used for result verification
  std::string access_token_retrieved_;
  std::string refresh_token_retrieved_;

 private:
  net::FakeURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<base::MessageLoopForIO> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(AccessTokenFetcherTest);
};

AccessTokenFetcherTest::AccessTokenFetcherTest()
    : url_fetcher_factory_(nullptr) {
}

AccessTokenFetcherTest::~AccessTokenFetcherTest() = default;

void AccessTokenFetcherTest::OnAccessTokenRetrieved(
    base::Closure done_closure,
    const std::string& access_token,
    const std::string& refresh_token) {
  access_token_retrieved_ = access_token;
  refresh_token_retrieved_ = refresh_token;

  done_closure.Run();
}

void AccessTokenFetcherTest::SetUp() {
  if (!base::MessageLoopCurrent::Get()) {
    // Create a temporary message loop if the current thread does not already
    // have one so we can use its task runner to create a request object.
    message_loop_.reset(new base::MessageLoopForIO);
  }
}

void AccessTokenFetcherTest::SetFakeResponse(
    const GURL& url,
    const std::string& data,
    net::HttpStatusCode code,
    net::URLRequestStatus::Status status) {
  url_fetcher_factory_.SetFakeResponse(url, data, code, status);
}

TEST_F(AccessTokenFetcherTest, ExchangeAuthCodeForAccessToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_info_url(),
                  kValidTokenInfoResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop.Run();

  EXPECT_EQ(access_token_retrieved_.compare(kAccessTokenValue), 0);
  EXPECT_EQ(refresh_token_retrieved_.compare(kRefreshTokenValue), 0);
}

TEST_F(AccessTokenFetcherTest, ExchangeRefreshTokenForAccessToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_info_url(),
                  kValidTokenInfoResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop.Run();

  EXPECT_EQ(access_token_retrieved_.compare(kAccessTokenValue), 0);
  EXPECT_EQ(refresh_token_retrieved_.compare(kRefreshTokenValue), 0);
}

TEST_F(AccessTokenFetcherTest, MultipleAccessTokenCalls) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_info_url(),
                  kValidTokenInfoResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  std::unique_ptr<base::RunLoop> run_loop;
  run_loop.reset(new base::RunLoop());
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop->QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop->Run();

  EXPECT_EQ(access_token_retrieved_.compare(kAccessTokenValue), 0);
  EXPECT_EQ(refresh_token_retrieved_.compare(kRefreshTokenValue), 0);

  // Reset our token data for the next iteration.
  access_token_retrieved_.clear();
  refresh_token_retrieved_.clear();

  // Update the response since we will call the refresh token method next.
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  run_loop.reset(new base::RunLoop());
  access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop->QuitClosure());

  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop->Run();

  EXPECT_EQ(access_token_retrieved_.compare(kAccessTokenValue), 0);
  EXPECT_EQ(refresh_token_retrieved_.compare(kRefreshTokenValue), 0);

  run_loop.reset(new base::RunLoop());
  access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop->QuitClosure());

  // Reset our token data for the next iteration.
  access_token_retrieved_.clear();
  refresh_token_retrieved_.clear();

  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop->Run();

  EXPECT_EQ(access_token_retrieved_.compare(kAccessTokenValue), 0);
  EXPECT_EQ(refresh_token_retrieved_.compare(kRefreshTokenValue), 0);
}

TEST_F(AccessTokenFetcherTest, ExchangeAuthCode_Unauthorized_Error) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeValidResponse, net::HTTP_UNAUTHORIZED,
                  net::URLRequestStatus::FAILED);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, ExchangeRefreshToken_Unauthorized_Error) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeValidResponse, net::HTTP_UNAUTHORIZED,
                  net::URLRequestStatus::FAILED);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, ExchangeAuthCode_NetworkError) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeValidResponse, net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::FAILED);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, ExchangeRefreshToken_NetworkError) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeValidResponse, net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::FAILED);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, AuthCode_GetTokenInfoResponse_InvalidToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_info_url(),
                  kInvalidTokenInfoResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, ExchangeAuthCodeForAccessToken_EmptyToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kAuthCodeExchangeEmptyResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromAuthCode(kAuthCodeValue,
                                                  access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, RefreshToken_GetTokenInfoResponse_InvalidToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeValidResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_info_url(),
                  kInvalidTokenInfoResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

TEST_F(AccessTokenFetcherTest, ExchangeRefreshTokenForAccessToken_EmptyToken) {
  SetFakeResponse(GaiaUrls::GetInstance()->oauth2_token_url(),
                  kRefreshTokenExchangeEmptyResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  AccessTokenCallback access_token_callback =
      base::Bind(&AccessTokenFetcherTest::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  AccessTokenFetcher access_token_fetcher;
  access_token_fetcher.GetAccessTokenFromRefreshToken(kRefreshTokenValue,
                                                      access_token_callback);

  run_loop.Run();

  // Our callback should have been called with empty strings.
  EXPECT_TRUE(access_token_retrieved_.empty());
  EXPECT_TRUE(refresh_token_retrieved_.empty());
}

}  // namespace test
}  // namespace remoting
