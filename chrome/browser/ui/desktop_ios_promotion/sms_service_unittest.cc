// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/desktop_ios_promotion/sms_service.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A testing web history service that does extra checks and creates a
// TestRequest instead of a normal request.
class TestingSMSService : public SMSService {
 public:
  explicit TestingSMSService(
      ProfileOAuth2TokenService* token_service,
      SigninManagerBase* signin_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : SMSService(token_service, signin_manager, url_loader_factory) {}

  ~TestingSMSService() override {}

  SMSService::Request* CreateRequest(
      const GURL& url,
      const CompletionCallback& callback) override;

  void SetNextRequestResponseData(int response_code,
                                  const std::string& response_body) {
    next_response_code_ = response_code;
    next_response_body_ = response_body;
  }

  void QueryPhoneNumberCallbackSucceeded(SMSService::Request* request,
                                         bool success,
                                         const std::string& number) {
    EXPECT_TRUE(success);
    EXPECT_EQ(number, "1");
  }

  void QueryPhoneNumberCallbackFailed(SMSService::Request* request,
                                      bool success,
                                      const std::string& number) {
    EXPECT_FALSE(success);
    EXPECT_EQ(number, "");
  }

 private:
  int next_response_code_;
  std::string next_response_body_;

  DISALLOW_COPY_AND_ASSIGN(TestingSMSService);
};

// A testing request class that allows expected values to be filled in.
class TestRequest : public SMSService::Request {
 public:
  TestRequest(const SMSService::CompletionCallback& callback,
              int response_code,
              const std::string& response_body)
      : callback_(callback),
        response_code_(response_code),
        response_body_(response_body),
        is_pending_(false) {}

  ~TestRequest() override {}

  // history::Request overrides
  bool IsPending() override { return is_pending_; }
  int GetResponseCode() override { return response_code_; }
  const std::string& GetResponseBody() override { return response_body_; }
  void SetPostData(const std::string& post_data) override {
    post_data_ = post_data;
  }
  void SetPostDataAndType(const std::string& post_data,
                          const std::string& mime_type) override {
    SetPostData(post_data);
  }
  void Start() override {
    is_pending_ = true;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&TestRequest::MimicReturnFromFetch, base::Unretained(this)));
  }

  void MimicReturnFromFetch() {
    callback_.Run(this, response_code_ == net::HTTP_OK);
  }

 private:
  GURL url_;
  SMSService::CompletionCallback callback_;
  int response_code_;
  std::string response_body_;
  std::string post_data_;
  bool is_pending_;

  DISALLOW_COPY_AND_ASSIGN(TestRequest);
};

SMSService::Request* TestingSMSService::CreateRequest(
    const GURL& url,
    const CompletionCallback& callback) {
  SMSService::Request* request =
      new TestRequest(callback, next_response_code_, next_response_body_);
  return request;
}

}  // namespace

// A test class used for testing the SMSService class.
// In order for SMSService to be valid, we must have a valid
// ProfileSyncService. Using the ProfileSyncServiceMock class allows to
// assign specific return values as needed to make sure the web history
// service is available.
class SMSServiceTest : public testing::Test {
 public:
  SMSServiceTest()
      : signin_client_(nullptr),
        signin_manager_(&signin_client_, &account_tracker_),
        test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)),
        sms_service_(&token_service_,
                     &signin_manager_,
                     test_shared_loader_factory_) {}

  ~SMSServiceTest() override {}

  void TearDown() override {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  run_loop.QuitClosure());
    run_loop.Run();
  }

  TestingSMSService* sms_service() { return &sms_service_; }

  void SetNextRequestResponseData(int response_code,
                                  const std::string& response_body) {
    sms_service_.SetNextRequestResponseData(response_code, response_body);
  }

 private:
  base::MessageLoop message_loop_;
  FakeProfileOAuth2TokenService token_service_;
  AccountTrackerService account_tracker_;
  TestSigninClient signin_client_;
  FakeSigninManagerBase signin_manager_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
  TestingSMSService sms_service_;

  DISALLOW_COPY_AND_ASSIGN(SMSServiceTest);
};

TEST_F(SMSServiceTest, VerifyJsonData) {
  // Test that properly formatted response with good response code returns true
  // as expected.
  std::string query_phone_valid =
      "{\n\"phoneNumber\": "
      "  [\n"
      "    {\"phoneNumber\": \"1\"},"
      "    {\"phoneNumber\": \"2\"}"
      "  ]\n"
      "}";
  sms_service()->SetNextRequestResponseData(net::HTTP_OK, query_phone_valid);
  sms_service()->QueryPhoneNumber(
      base::Bind(&TestingSMSService::QueryPhoneNumberCallbackSucceeded,
                 base::Unretained(sms_service())));
  base::RunLoop().RunUntilIdle();
  std::string send_sms_valid = "{\"phoneNumber\": \"1\"}";
  sms_service()->SetNextRequestResponseData(net::HTTP_OK, send_sms_valid);
  std::string promo_id = "";
  sms_service()->SendSMS(
      promo_id,
      base::Bind(&TestingSMSService::QueryPhoneNumberCallbackSucceeded,
                 base::Unretained(sms_service())));
  base::RunLoop().RunUntilIdle();
  // Test that improperly formatted response returns no number.
  std::string query_phone_invalid =
      "{\n\"phoneNumber\": "
      "  [\n"
      "  ]\n"
      "}";
  sms_service()->SetNextRequestResponseData(net::HTTP_OK, query_phone_invalid);
  sms_service()->QueryPhoneNumber(
      base::Bind(&TestingSMSService::QueryPhoneNumberCallbackFailed,
                 base::Unretained(sms_service())));
  base::RunLoop().RunUntilIdle();
  std::string send_sms_invalid = "{}";
  sms_service()->SetNextRequestResponseData(net::HTTP_OK, send_sms_invalid);
  sms_service()->SendSMS(
      promo_id, base::Bind(&TestingSMSService::QueryPhoneNumberCallbackFailed,
                           base::Unretained(sms_service())));
  base::RunLoop().RunUntilIdle();
}
