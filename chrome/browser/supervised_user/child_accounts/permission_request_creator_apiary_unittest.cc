// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/child_accounts/permission_request_creator_apiary.h"

#include <memory>
#include <utility>

#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kAccountId[] = "account@gmail.com";

std::string BuildResponse() {
  base::DictionaryValue dict;
  auto permission_dict = std::make_unique<base::DictionaryValue>();
  permission_dict->SetKey("id", base::Value("requestid"));
  dict.SetWithoutPathExpansion("permissionRequest", std::move(permission_dict));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

}  // namespace

class PermissionRequestCreatorApiaryTest : public testing::Test {
 public:
  PermissionRequestCreatorApiaryTest()
      : test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)),
        permission_creator_(&token_service_,
                            kAccountId,
                            test_shared_loader_factory_) {
    permission_creator_.retry_on_network_change_ = false;
    token_service_.UpdateCredentials(kAccountId, "refresh_token");
  }

 protected:
  void IssueAccessTokens() {
    token_service_.IssueAllTokensForAccount(
        kAccountId,
        "access_token",
        base::Time::Now() + base::TimeDelta::FromHours(1));
  }

  void IssueAccessTokenErrors() {
    token_service_.IssueErrorForAllPendingRequestsForAccount(
        kAccountId,
        GoogleServiceAuthError::FromServiceError("Error!"));
  }

  void SetupResponse(net::Error error, const std::string& response) {
    network::ResourceResponseHead head;
    std::string headers("HTTP/1.1 200 OK\n\n");
    head.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.size()));
    network::URLLoaderCompletionStatus status(error);
    status.decoded_body_length = response.size();
    test_url_loader_factory_.AddResponse(permission_creator_.GetApiUrl(), head,
                                         response, status);
  }

  void CreateRequest(const GURL& url) {
    permission_creator_.CreateURLAccessRequest(
        url,
        base::BindOnce(&PermissionRequestCreatorApiaryTest::OnRequestCreated,
                       base::Unretained(this)));
  }

  void WaitForResponse() { base::RunLoop().RunUntilIdle(); }

  MOCK_METHOD1(OnRequestCreated, void(bool success));

  base::MessageLoop message_loop_;
  FakeProfileOAuth2TokenService token_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
  PermissionRequestCreatorApiary permission_creator_;
};

TEST_F(PermissionRequestCreatorApiaryTest, Success) {
  CreateRequest(GURL("http://randomurl.com"));
  CreateRequest(GURL("http://anotherurl.com"));

  // We should have gotten a request for an access token.
  EXPECT_GT(token_service_.GetPendingRequests().size(), 0U);

  IssueAccessTokens();

  EXPECT_CALL(*this, OnRequestCreated(true)).Times(2);
  SetupResponse(net::OK, BuildResponse());
  SetupResponse(net::OK, BuildResponse());
  WaitForResponse();
}

TEST_F(PermissionRequestCreatorApiaryTest, AccessTokenError) {
  CreateRequest(GURL("http://randomurl.com"));

  // We should have gotten a request for an access token.
  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  // Our callback should get called immediately on an error.
  EXPECT_CALL(*this, OnRequestCreated(false));
  IssueAccessTokenErrors();
}

TEST_F(PermissionRequestCreatorApiaryTest, NetworkError) {
  const GURL& url = GURL("http://randomurl.com");
  CreateRequest(url);

  // We should have gotten a request for an access token.
  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  IssueAccessTokens();

  // Our callback should get called on an error.
  EXPECT_CALL(*this, OnRequestCreated(false));

  SetupResponse(net::ERR_ABORTED, std::string());
  WaitForResponse();
}
