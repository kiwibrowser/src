// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/android_management_client.h"
#include "components/policy/core/common/cloud/mock_device_management_service.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::SaveArg;
using testing::StrictMock;
using testing::_;

namespace em = enterprise_management;

namespace policy {

namespace {

const char kAccountId[] = "fake-account-id";
const char kRefreshToken[] = "fake-refresh-token";
const char kOAuthToken[] = "fake-oauth-token";

MATCHER_P(MatchProto, expected, "matches protobuf") {
  return arg.SerializePartialAsString() == expected.SerializePartialAsString();
}

}  // namespace

class AndroidManagementClientTest : public testing::Test {
 protected:
  AndroidManagementClientTest() {
    android_management_request_.mutable_check_android_management_request();
    android_management_response_.mutable_check_android_management_response();
  }

  // testing::Test:
  void SetUp() override {
    request_context_ =
        new net::TestURLRequestContextGetter(loop_.task_runner());
    client_.reset(new AndroidManagementClient(&service_, request_context_,
                                              kAccountId, &token_service_));
  }

  // Request protobuf is used as extectation for the client requests.
  em::DeviceManagementRequest android_management_request_;

  // Protobuf is used in successfil responsees.
  em::DeviceManagementResponse android_management_response_;

  base::MessageLoop loop_;
  MockDeviceManagementService service_;
  StrictMock<base::MockCallback<AndroidManagementClient::StatusCallback>>
      callback_observer_;
  std::unique_ptr<AndroidManagementClient> client_;
  // Pointer to the client's request context.
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  std::string oauh_token_;
  FakeProfileOAuth2TokenService token_service_;
};

TEST_F(AndroidManagementClientTest, CheckAndroidManagementCall) {
  std::string client_id;
  EXPECT_CALL(
      service_,
      CreateJob(DeviceManagementRequestJob::TYPE_ANDROID_MANAGEMENT_CHECK,
                request_context_))
      .WillOnce(service_.SucceedJob(android_management_response_));
  EXPECT_CALL(service_,
              StartJob(dm_protocol::kValueRequestCheckAndroidManagement,
                       std::string(), kOAuthToken, std::string(), _,
                       MatchProto(android_management_request_)))
      .WillOnce(SaveArg<4>(&client_id));
  EXPECT_CALL(callback_observer_,
              Run(AndroidManagementClient::Result::UNMANAGED))
      .Times(1);

  token_service_.UpdateCredentials(kAccountId, kRefreshToken);
  client_->StartCheckAndroidManagement(callback_observer_.Get());
  token_service_.IssueAllTokensForAccount(kAccountId, kOAuthToken,
                                          base::Time::Max());
  ASSERT_LT(client_id.size(), 64U);
}

}  // namespace policy
