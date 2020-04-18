// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "google_apis/gaia/fake_oauth2_token_service_delegate.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_access_token_consumer.h"
#include "google_apis/gaia/oauth2_access_token_fetcher_impl.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "google_apis/gaia/oauth2_token_service_test_util.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// A testing consumer that retries on error.
class RetryingTestingOAuth2TokenServiceConsumer
    : public TestingOAuth2TokenServiceConsumer {
 public:
  RetryingTestingOAuth2TokenServiceConsumer(
      OAuth2TokenService* oauth2_service,
      const std::string& account_id)
      : oauth2_service_(oauth2_service),
        account_id_(account_id) {}
  ~RetryingTestingOAuth2TokenServiceConsumer() override {}

  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override {
    TestingOAuth2TokenServiceConsumer::OnGetTokenFailure(request, error);
    request_ = oauth2_service_->StartRequest(
        account_id_, OAuth2TokenService::ScopeSet(), this);
  }

  OAuth2TokenService* oauth2_service_;
  std::string account_id_;
  std::unique_ptr<OAuth2TokenService::Request> request_;
};

class TestOAuth2TokenService : public OAuth2TokenService {
 public:
  explicit TestOAuth2TokenService(
      std::unique_ptr<FakeOAuth2TokenServiceDelegate> delegate)
      : OAuth2TokenService(std::move(delegate)) {}

  void CancelAllRequestsForTest() { CancelAllRequests(); }

  void CancelRequestsForAccountForTest(const std::string& account_id) {
    CancelRequestsForAccount(account_id);
  }

  FakeOAuth2TokenServiceDelegate* GetFakeOAuth2TokenServiceDelegate() {
    return static_cast<FakeOAuth2TokenServiceDelegate*>(GetDelegate());
  }
};

class OAuth2TokenServiceTest : public testing::Test {
 public:
  void SetUp() override {
    oauth2_service_.reset(new TestOAuth2TokenService(
        std::make_unique<FakeOAuth2TokenServiceDelegate>(
            new net::TestURLRequestContextGetter(
                message_loop_.task_runner()))));
    account_id_ = "test_user@gmail.com";
  }

  void TearDown() override {
    // Makes sure that all the clean up tasks are run.
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::MessageLoopForIO message_loop_;  // net:: stuff needs IO message loop.
  net::TestURLFetcherFactory factory_;
  std::unique_ptr<TestOAuth2TokenService> oauth2_service_;
  std::string account_id_;
  TestingOAuth2TokenServiceConsumer consumer_;
};

TEST_F(OAuth2TokenServiceTest, NoOAuth2RefreshToken) {
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, FailureShouldNotRetry) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_UNAUTHORIZED);
  fetcher->SetResponseString(std::string());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer_.number_of_errors_);
  EXPECT_EQ(fetcher, factory_.GetFetcherByID(0));
}

TEST_F(OAuth2TokenServiceTest, SuccessWithoutCaching) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, SuccessWithCaching) {
  OAuth2TokenService::ScopeSet scopes1;
  scopes1.insert("s1");
  scopes1.insert("s2");
  OAuth2TokenService::ScopeSet scopes1_same;
  scopes1_same.insert("s2");
  scopes1_same.insert("s1");
  OAuth2TokenService::ScopeSet scopes2;
  scopes2.insert("s3");

  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  // First request.
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, scopes1, &consumer_));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Second request to the same set of scopes, should return the same token
  // without needing a network request.
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, scopes1_same, &consumer_));
  base::RunLoop().RunUntilIdle();

  // No new network fetcher.
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Third request to a new set of scopes, should return another token.
  std::unique_ptr<OAuth2TokenService::Request> request3(
      oauth2_service_->StartRequest(account_id_, scopes2, &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token2", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(3, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token2", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, SuccessAndExpirationAndFailure) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  // First request.
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 0));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Second request must try to access the network as the token has expired.
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);

  // Network failure.
  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_UNAUTHORIZED);
  fetcher->SetResponseString(std::string());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, SuccessAndExpirationAndSuccess) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  // First request.
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 0));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Second request must try to access the network as the token has expired.
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);

  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("another token", 0));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("another token", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, RequestDeletedBeforeCompletion) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  request.reset();

  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, RequestDeletedAfterCompletion) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  request.reset();

  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, MultipleRequestsForTheSameScopesWithOneDeleted) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();

  request.reset();

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, ClearedRefreshTokenFailsSubsequentRequests) {
  // We have a valid refresh token; the first request is successful.
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // The refresh token is no longer available; subsequent requests fail.
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "");
  request = oauth2_service_->StartRequest(account_id_,
      OAuth2TokenService::ScopeSet(), &consumer_);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest,
       ChangedRefreshTokenDoesNotAffectInFlightRequests) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "first refreshToken");
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert("s1");
  scopes.insert("s2");
  OAuth2TokenService::ScopeSet scopes1;
  scopes.insert("s3");
  scopes.insert("s4");

  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, scopes, &consumer_));
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher1 = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher1);

  // Note |request| is still pending when the refresh token changes.
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "second refreshToken");

  // A 2nd request (using the new refresh token) that occurs and completes
  // while the 1st request is in flight is successful.
  TestingOAuth2TokenServiceConsumer consumer2;
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, scopes1, &consumer2));
  base::RunLoop().RunUntilIdle();

  net::TestURLFetcher* fetcher2 = factory_.GetFetcherByID(0);
  fetcher2->set_response_code(net::HTTP_OK);
  fetcher2->SetResponseString(GetValidTokenResponse("second token", 3600));
  fetcher2->delegate()->OnURLFetchComplete(fetcher2);
  EXPECT_EQ(1, consumer2.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer2.number_of_errors_);
  EXPECT_EQ("second token", consumer2.last_token_);

  fetcher1->set_response_code(net::HTTP_OK);
  fetcher1->SetResponseString(GetValidTokenResponse("first token", 3600));
  fetcher1->delegate()->OnURLFetchComplete(fetcher1);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("first token", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, ServiceShutDownBeforeFetchComplete) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);

  // The destructor should cancel all in-flight fetchers.
  oauth2_service_.reset(NULL);

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, RetryingConsumer) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  RetryingTestingOAuth2TokenServiceConsumer consumer(oauth2_service_.get(),
      account_id_);
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer.number_of_errors_);

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_UNAUTHORIZED);
  fetcher->SetResponseString(std::string());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0, consumer.number_of_successful_tokens_);
  EXPECT_EQ(1, consumer.number_of_errors_);

  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_UNAUTHORIZED);
  fetcher->SetResponseString(std::string());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0, consumer.number_of_successful_tokens_);
  EXPECT_EQ(2, consumer.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, InvalidateToken) {
  OAuth2TokenService::ScopeSet scopes;
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");

  // First request.
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, scopes, &consumer_));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Second request, should return the same token without needing a network
  // request.
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, scopes, &consumer_));
  base::RunLoop().RunUntilIdle();

  // No new network fetcher.
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);

  // Invalidating the token should return a new token on the next request.
  oauth2_service_->InvalidateAccessToken(account_id_, scopes,
                                         consumer_.last_token_);
  std::unique_ptr<OAuth2TokenService::Request> request3(
      oauth2_service_->StartRequest(account_id_, scopes, &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token2", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(3, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token2", consumer_.last_token_);
}

TEST_F(OAuth2TokenServiceTest, CancelAllRequests) {
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));

  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      "account_id_2", "refreshToken2");
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, OAuth2TokenService::ScopeSet(),
                                    &consumer_));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);

  oauth2_service_->CancelAllRequestsForTest();

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(2, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, CancelRequestsForAccount) {
  OAuth2TokenService::ScopeSet scope_set_1;
  scope_set_1.insert("scope1");
  scope_set_1.insert("scope2");
  OAuth2TokenService::ScopeSet scope_set_2(scope_set_1.begin(),
                                           scope_set_1.end());
  scope_set_2.insert("scope3");

  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request1(
      oauth2_service_->StartRequest(account_id_, scope_set_1, &consumer_));
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequest(account_id_, scope_set_2, &consumer_));

  std::string account_id_2("account_id_2");
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_2, "refreshToken2");
  std::unique_ptr<OAuth2TokenService::Request> request3(
      oauth2_service_->StartRequest(account_id_2, scope_set_1, &consumer_));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);

  oauth2_service_->CancelRequestsForAccountForTest(account_id_);

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(2, consumer_.number_of_errors_);

  oauth2_service_->CancelRequestsForAccountForTest(account_id_2);

  EXPECT_EQ(0, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(3, consumer_.number_of_errors_);
}

TEST_F(OAuth2TokenServiceTest, SameScopesRequestedForDifferentClients) {
  std::string client_id_1("client1");
  std::string client_secret_1("secret1");
  std::string client_id_2("client2");
  std::string client_secret_2("secret2");
  std::set<std::string> scope_set;
  scope_set.insert("scope1");
  scope_set.insert("scope2");

  std::string refresh_token("refreshToken");
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      account_id_, refresh_token);

  std::unique_ptr<OAuth2TokenService::Request> request1(
      oauth2_service_->StartRequestForClient(
          account_id_, client_id_1, client_secret_1, scope_set, &consumer_));
  std::unique_ptr<OAuth2TokenService::Request> request2(
      oauth2_service_->StartRequestForClient(
          account_id_, client_id_2, client_secret_2, scope_set, &consumer_));
  // Start a request that should be duplicate of |request1|.
  std::unique_ptr<OAuth2TokenService::Request> request3(
      oauth2_service_->StartRequestForClient(
          account_id_, client_id_1, client_secret_1, scope_set, &consumer_));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(2U,
            oauth2_service_->GetNumPendingRequestsForTesting(
                client_id_1,
                account_id_,
                scope_set));
   ASSERT_EQ(1U,
             oauth2_service_->GetNumPendingRequestsForTesting(
                client_id_2,
                account_id_,
                scope_set));
}

TEST_F(OAuth2TokenServiceTest, RequestParametersOrderTest) {
  OAuth2TokenService::ScopeSet set_0;
  OAuth2TokenService::ScopeSet set_1;
  set_1.insert("1");

  OAuth2TokenService::RequestParameters params[] = {
      OAuth2TokenService::RequestParameters("0", "0", set_0),
      OAuth2TokenService::RequestParameters("0", "0", set_1),
      OAuth2TokenService::RequestParameters("0", "1", set_0),
      OAuth2TokenService::RequestParameters("0", "1", set_1),
      OAuth2TokenService::RequestParameters("1", "0", set_0),
      OAuth2TokenService::RequestParameters("1", "0", set_1),
      OAuth2TokenService::RequestParameters("1", "1", set_0),
      OAuth2TokenService::RequestParameters("1", "1", set_1),
  };

  for (size_t i = 0; i < arraysize(params); i++) {
    for (size_t j = 0; j < arraysize(params); j++) {
      if (i == j) {
        EXPECT_FALSE(params[i] < params[j]) << " i=" << i << ", j=" << j;
        EXPECT_FALSE(params[j] < params[i]) << " i=" << i << ", j=" << j;
      } else if (i < j) {
        EXPECT_TRUE(params[i] < params[j]) << " i=" << i << ", j=" << j;
        EXPECT_FALSE(params[j] < params[i]) << " i=" << i << ", j=" << j;
      } else {
        EXPECT_TRUE(params[j] < params[i]) << " i=" << i << ", j=" << j;
        EXPECT_FALSE(params[i] < params[j]) << " i=" << i << ", j=" << j;
      }
    }
  }
}

TEST_F(OAuth2TokenServiceTest, UpdateClearsCache) {
  std::string kEmail = "test@gmail.com";
  std::set<std::string> scope_list;
  scope_list.insert("scope");
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      kEmail, "refreshToken");
  std::unique_ptr<OAuth2TokenService::Request> request(
      oauth2_service_->StartRequest(kEmail, scope_list, &consumer_));
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("token", consumer_.last_token_);
  EXPECT_EQ(1, (int)oauth2_service_->token_cache_.size());

  // Signs out and signs in
  oauth2_service_->RevokeAllCredentials();

  EXPECT_EQ(0, (int)oauth2_service_->token_cache_.size());
  oauth2_service_->GetFakeOAuth2TokenServiceDelegate()->UpdateCredentials(
      kEmail, "refreshToken");
  request = oauth2_service_->StartRequest(kEmail, scope_list, &consumer_);
  base::RunLoop().RunUntilIdle();
  fetcher = factory_.GetFetcherByID(0);
  // ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenResponse("another token", 3600));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(2, consumer_.number_of_successful_tokens_);
  EXPECT_EQ(0, consumer_.number_of_errors_);
  EXPECT_EQ("another token", consumer_.last_token_);
  EXPECT_EQ(1, (int)oauth2_service_->token_cache_.size());
}
