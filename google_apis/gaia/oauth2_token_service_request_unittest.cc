// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_token_service_request.h"

#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kAccessToken[] = "access_token";
const char kAccountId[] = "test_user@gmail.com";
const char kScope[] = "SCOPE";

class TestingOAuth2TokenServiceConsumer : public OAuth2TokenService::Consumer {
 public:
  TestingOAuth2TokenServiceConsumer();
  ~TestingOAuth2TokenServiceConsumer() override;

  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  int num_get_token_success_;
  int num_get_token_failure_;
  std::string last_token_;
  GoogleServiceAuthError last_error_;
};

TestingOAuth2TokenServiceConsumer::TestingOAuth2TokenServiceConsumer()
    : OAuth2TokenService::Consumer("test"),
      num_get_token_success_(0),
      num_get_token_failure_(0),
      last_error_(GoogleServiceAuthError::AuthErrorNone()) {
}

TestingOAuth2TokenServiceConsumer::~TestingOAuth2TokenServiceConsumer() {
}

void TestingOAuth2TokenServiceConsumer::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& token,
    const base::Time& expiration_date) {
  last_token_ = token;
  ++num_get_token_success_;
}

void TestingOAuth2TokenServiceConsumer::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  last_error_ = error;
  ++num_get_token_failure_;
}

// A mock implementation of an OAuth2TokenService.
//
// Use SetResponse to vary the response to token requests.
class MockOAuth2TokenService : public FakeOAuth2TokenService {
 public:
  MockOAuth2TokenService();
  ~MockOAuth2TokenService() override;

  void SetResponse(const GoogleServiceAuthError& error,
                   const std::string& access_token,
                   const base::Time& expiration);

  int num_invalidate_token() const { return num_invalidate_token_; }

  const std::string& last_token_invalidated() const {
    return last_token_invalidated_;
  }

 protected:
  void FetchOAuth2Token(RequestImpl* request,
                        const std::string& account_id,
                        net::URLRequestContextGetter* getter,
                        const std::string& client_id,
                        const std::string& client_secret,
                        const ScopeSet& scopes) override;

  void InvalidateAccessTokenImpl(const std::string& account_id,
                                 const std::string& client_id,
                                 const ScopeSet& scopes,
                                 const std::string& access_token) override;

 private:
  GoogleServiceAuthError response_error_;
  std::string response_access_token_;
  base::Time response_expiration_;
  int num_invalidate_token_;
  std::string last_token_invalidated_;
};

MockOAuth2TokenService::MockOAuth2TokenService()
    : response_error_(GoogleServiceAuthError::AuthErrorNone()),
      response_access_token_(kAccessToken),
      response_expiration_(base::Time::Max()),
      num_invalidate_token_(0) {
}

MockOAuth2TokenService::~MockOAuth2TokenService() {
}

void MockOAuth2TokenService::SetResponse(const GoogleServiceAuthError& error,
                                         const std::string& access_token,
                                         const base::Time& expiration) {
  response_error_ = error;
  response_access_token_ = access_token;
  response_expiration_ = expiration;
}

void MockOAuth2TokenService::FetchOAuth2Token(
    RequestImpl* request,
    const std::string& account_id,
    net::URLRequestContextGetter* getter,
    const std::string& client_id,
    const std::string& client_secret,
    const ScopeSet& scopes) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&OAuth2TokenService::RequestImpl::InformConsumer,
                            request->AsWeakPtr(), response_error_,
                            response_access_token_, response_expiration_));
}

void MockOAuth2TokenService::InvalidateAccessTokenImpl(
    const std::string& account_id,
    const std::string& client_id,
    const ScopeSet& scopes,
    const std::string& access_token) {
  ++num_invalidate_token_;
  last_token_invalidated_ = access_token;
}

class OAuth2TokenServiceRequestTest : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  class Provider : public OAuth2TokenServiceRequest::TokenServiceProvider {
   public:
    Provider(const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
             OAuth2TokenService* token_service);

    scoped_refptr<base::SingleThreadTaskRunner> GetTokenServiceTaskRunner()
        override;
    OAuth2TokenService* GetTokenService() override;

   private:
    ~Provider() override;

    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
    OAuth2TokenService* token_service_;
  };

  base::MessageLoop ui_loop_;
  OAuth2TokenService::ScopeSet scopes_;
  std::unique_ptr<MockOAuth2TokenService> oauth2_service_;
  scoped_refptr<OAuth2TokenServiceRequest::TokenServiceProvider> provider_;
  TestingOAuth2TokenServiceConsumer consumer_;
};

void OAuth2TokenServiceRequestTest::SetUp() {
  scopes_.insert(kScope);
  oauth2_service_.reset(new MockOAuth2TokenService);
  oauth2_service_->AddAccount(kAccountId);
  provider_ =
      new Provider(base::ThreadTaskRunnerHandle::Get(), oauth2_service_.get());
}

void OAuth2TokenServiceRequestTest::TearDown() {
  // Run the loop to execute any pending tasks that may free resources.
  base::RunLoop().RunUntilIdle();
}

OAuth2TokenServiceRequestTest::Provider::Provider(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    OAuth2TokenService* token_service)
    : task_runner_(task_runner), token_service_(token_service) {
}

scoped_refptr<base::SingleThreadTaskRunner>
OAuth2TokenServiceRequestTest::Provider::GetTokenServiceTaskRunner() {
  return task_runner_;
}

OAuth2TokenService* OAuth2TokenServiceRequestTest::Provider::GetTokenService() {
  return token_service_;
}

OAuth2TokenServiceRequestTest::Provider::~Provider() {
}

TEST_F(OAuth2TokenServiceRequestTest, CreateAndStart_Failure) {
  oauth2_service_->SetResponse(
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE),
      std::string(),
      base::Time());
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      OAuth2TokenServiceRequest::CreateAndStart(provider_.get(), kAccountId,
                                                scopes_, &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.num_get_token_success_);
  EXPECT_EQ(1, consumer_.num_get_token_failure_);
  EXPECT_EQ(GoogleServiceAuthError::SERVICE_UNAVAILABLE,
            consumer_.last_error_.state());
  EXPECT_EQ(0, oauth2_service_->num_invalidate_token());
}

TEST_F(OAuth2TokenServiceRequestTest, CreateAndStart_Success) {
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      OAuth2TokenServiceRequest::CreateAndStart(provider_.get(), kAccountId,
                                                scopes_, &consumer_));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, consumer_.num_get_token_success_);
  EXPECT_EQ(0, consumer_.num_get_token_failure_);
  EXPECT_EQ(kAccessToken, consumer_.last_token_);
  EXPECT_EQ(0, oauth2_service_->num_invalidate_token());
}

TEST_F(OAuth2TokenServiceRequestTest,
       CreateAndStart_DestroyRequestBeforeCompletes) {
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      OAuth2TokenServiceRequest::CreateAndStart(provider_.get(), kAccountId,
                                                scopes_, &consumer_));
  request.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.num_get_token_success_);
  EXPECT_EQ(0, consumer_.num_get_token_failure_);
  EXPECT_EQ(0, oauth2_service_->num_invalidate_token());
}

TEST_F(OAuth2TokenServiceRequestTest,
       CreateAndStart_DestroyRequestAfterCompletes) {
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      OAuth2TokenServiceRequest::CreateAndStart(provider_.get(), kAccountId,
                                                scopes_, &consumer_));
  base::RunLoop().RunUntilIdle();
  request.reset();
  EXPECT_EQ(1, consumer_.num_get_token_success_);
  EXPECT_EQ(0, consumer_.num_get_token_failure_);
  EXPECT_EQ(kAccessToken, consumer_.last_token_);
  EXPECT_EQ(0, oauth2_service_->num_invalidate_token());
}

TEST_F(OAuth2TokenServiceRequestTest, InvalidateToken) {
  OAuth2TokenServiceRequest::InvalidateToken(
      provider_.get(), kAccountId, scopes_, kAccessToken);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, consumer_.num_get_token_success_);
  EXPECT_EQ(0, consumer_.num_get_token_failure_);
  EXPECT_EQ(kAccessToken, oauth2_service_->last_token_invalidated());
  EXPECT_EQ(1, oauth2_service_->num_invalidate_token());
}

}  // namespace
