// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/child_accounts/family_info_fetcher.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kAccountId[] = "user@gmail.com";
const char kDifferentAccountId[] = "some_other_user@gmail.com";

bool operator==(const FamilyInfoFetcher::FamilyProfile& family1,
                const FamilyInfoFetcher::FamilyProfile& family2) {
  return family1.id == family2.id &&
         family1.name == family2.name;
}

bool operator==(const FamilyInfoFetcher::FamilyMember& account1,
                const FamilyInfoFetcher::FamilyMember& account2) {
  return account1.obfuscated_gaia_id == account2.obfuscated_gaia_id &&
         account1.role == account2.role &&
         account1.display_name == account2.display_name &&
         account1.email == account2.email &&
         account1.profile_url == account2.profile_url &&
         account1.profile_image_url == account2.profile_image_url;
}

namespace {

std::string BuildGetFamilyProfileResponse(
    const FamilyInfoFetcher::FamilyProfile& family) {
  base::DictionaryValue dict;
  auto family_dict = std::make_unique<base::DictionaryValue>();
  family_dict->SetKey("familyId", base::Value(family.id));
  std::unique_ptr<base::DictionaryValue> profile_dict =
      std::make_unique<base::DictionaryValue>();
  profile_dict->SetKey("name", base::Value(family.name));
  family_dict->SetWithoutPathExpansion("profile", std::move(profile_dict));
  dict.SetWithoutPathExpansion("family", std::move(family_dict));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

std::string BuildEmptyGetFamilyProfileResponse() {
  base::DictionaryValue dict;
  dict.SetWithoutPathExpansion("family",
                               std::make_unique<base::DictionaryValue>());
  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

std::string BuildGetFamilyMembersResponse(
    const std::vector<FamilyInfoFetcher::FamilyMember>& members) {
  base::DictionaryValue dict;
  auto list = std::make_unique<base::ListValue>();
  for (size_t i = 0; i < members.size(); i++) {
    const FamilyInfoFetcher::FamilyMember& member = members[i];
    std::unique_ptr<base::DictionaryValue> member_dict(
        new base::DictionaryValue);
    member_dict->SetKey("userId", base::Value(member.obfuscated_gaia_id));
    member_dict->SetKey(
        "role", base::Value(FamilyInfoFetcher::RoleToString(member.role)));
    if (!member.display_name.empty() ||
        !member.email.empty() ||
        !member.profile_url.empty() ||
        !member.profile_image_url.empty()) {
      auto profile_dict = std::make_unique<base::DictionaryValue>();
      if (!member.display_name.empty())
        profile_dict->SetKey("displayName", base::Value(member.display_name));
      if (!member.email.empty())
        profile_dict->SetKey("email", base::Value(member.email));
      if (!member.profile_url.empty())
        profile_dict->SetKey("profileUrl", base::Value(member.profile_url));
      if (!member.profile_image_url.empty())
        profile_dict->SetKey("profileImageUrl",
                             base::Value(member.profile_image_url));

      member_dict->SetWithoutPathExpansion("profile", std::move(profile_dict));
    }
    list->Append(std::move(member_dict));
  }
  dict.SetWithoutPathExpansion("members", std::move(list));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

} // namespace

class FamilyInfoFetcherTest : public testing::Test,
                              public FamilyInfoFetcher::Consumer {
 public:
  FamilyInfoFetcherTest()
      : fetcher_(
            this,
            kAccountId,
            &token_service_,
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

  MOCK_METHOD1(OnGetFamilyProfileSuccess,
               void(const FamilyInfoFetcher::FamilyProfile& family));
  MOCK_METHOD1(OnGetFamilyMembersSuccess,
               void(const std::vector<FamilyInfoFetcher::FamilyMember>&
                        members));
  MOCK_METHOD1(OnFailure, void(FamilyInfoFetcher::ErrorCode error));

 protected:
  void IssueRefreshToken() {
    token_service_.UpdateCredentials(kAccountId, "refresh_token");
  }

  void IssueRefreshTokenForDifferentAccount() {
    token_service_.UpdateCredentials(kDifferentAccountId, "refresh_token");
  }

  void IssueAccessToken() {
    token_service_.IssueAllTokensForAccount(
        kAccountId,
        "access_token",
        base::Time::Now() + base::TimeDelta::FromHours(1));
  }

  void SendResponse(net::Error error, const std::string& response) {
    fetcher_.OnSimpleLoaderCompleteInternal(error, net::HTTP_OK, response);
  }

  void SendValidGetFamilyProfileResponse(
      const FamilyInfoFetcher::FamilyProfile& family) {
    SendResponse(net::OK, BuildGetFamilyProfileResponse(family));
  }

  void SendValidGetFamilyMembersResponse(
      const std::vector<FamilyInfoFetcher::FamilyMember>& members) {
    SendResponse(net::OK, BuildGetFamilyMembersResponse(members));
  }

  void SendInvalidGetFamilyProfileResponse() {
    SendResponse(net::OK, BuildEmptyGetFamilyProfileResponse());
  }

  void SendFailedResponse() {
    SendResponse(net::ERR_ABORTED, std::string());
  }

  base::MessageLoop message_loop_;
  FakeProfileOAuth2TokenService token_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  FamilyInfoFetcher fetcher_;
};


TEST_F(FamilyInfoFetcherTest, GetFamilyProfileSuccess) {
  IssueRefreshToken();

  fetcher_.StartGetFamilyProfile();

  // Since a refresh token is already available, we should immediately get a
  // request for an access token.
  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  IssueAccessToken();

  FamilyInfoFetcher::FamilyProfile family("test", "My Test Family");
  EXPECT_CALL(*this, OnGetFamilyProfileSuccess(family));
  SendValidGetFamilyProfileResponse(family);
}

TEST_F(FamilyInfoFetcherTest, GetFamilyMembersSuccess) {
  IssueRefreshToken();

  fetcher_.StartGetFamilyMembers();

  // Since a refresh token is already available, we should immediately get a
  // request for an access token.
  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  IssueAccessToken();

  std::vector<FamilyInfoFetcher::FamilyMember> members;
  members.push_back(
      FamilyInfoFetcher::FamilyMember("someObfuscatedGaiaId",
                                      FamilyInfoFetcher::HEAD_OF_HOUSEHOLD,
                                      "Homer Simpson",
                                      "homer@simpson.com",
                                      "http://profile.url/homer",
                                      "http://profile.url/homer/image"));
  members.push_back(
      FamilyInfoFetcher::FamilyMember("anotherObfuscatedGaiaId",
                                      FamilyInfoFetcher::PARENT,
                                      "Marge Simpson",
                                      std::string(),
                                      "http://profile.url/marge",
                                      std::string()));
  members.push_back(
      FamilyInfoFetcher::FamilyMember("obfuscatedGaiaId3",
                                      FamilyInfoFetcher::CHILD,
                                      "Lisa Simpson",
                                      "lisa@gmail.com",
                                      std::string(),
                                      "http://profile.url/lisa/image"));
  members.push_back(
      FamilyInfoFetcher::FamilyMember("obfuscatedGaiaId4",
                                      FamilyInfoFetcher::CHILD,
                                      "Bart Simpson",
                                      "bart@bart.bart",
                                      std::string(),
                                      std::string()));
  members.push_back(
      FamilyInfoFetcher::FamilyMember("obfuscatedGaiaId5",
                                      FamilyInfoFetcher::MEMBER,
                                      std::string(),
                                      std::string(),
                                      std::string(),
                                      std::string()));

  EXPECT_CALL(*this, OnGetFamilyMembersSuccess(members));
  SendValidGetFamilyMembersResponse(members);
}


TEST_F(FamilyInfoFetcherTest, SuccessAfterWaitingForRefreshToken) {
  fetcher_.StartGetFamilyProfile();

  // Since there is no refresh token yet, we should not get a request for an
  // access token at this point.
  EXPECT_EQ(0U, token_service_.GetPendingRequests().size());

  IssueRefreshToken();

  // Now there is a refresh token and we should have got a request for an
  // access token.
  EXPECT_EQ(1U, token_service_.GetPendingRequests().size());

  IssueAccessToken();

  FamilyInfoFetcher::FamilyProfile family("test", "My Test Family");
  EXPECT_CALL(*this, OnGetFamilyProfileSuccess(family));
  SendValidGetFamilyProfileResponse(family);
}

TEST_F(FamilyInfoFetcherTest, NoRefreshToken) {
  fetcher_.StartGetFamilyProfile();

  IssueRefreshTokenForDifferentAccount();

  // Credentials for a different user should be ignored, i.e. not result in a
  // request for an access token.
  EXPECT_EQ(0U, token_service_.GetPendingRequests().size());

  // After all refresh tokens have been loaded, there is still no token for our
  // user, so we expect a token error.
  EXPECT_CALL(*this, OnFailure(FamilyInfoFetcher::TOKEN_ERROR));
  token_service_.LoadCredentials("");
}

TEST_F(FamilyInfoFetcherTest, GetTokenFailure) {
  IssueRefreshToken();

  fetcher_.StartGetFamilyProfile();

  // On failure to get an access token we expect a token error.
  EXPECT_CALL(*this, OnFailure(FamilyInfoFetcher::TOKEN_ERROR));
  token_service_.IssueErrorForAllPendingRequestsForAccount(
      kAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
}

TEST_F(FamilyInfoFetcherTest, InvalidResponse) {
  IssueRefreshToken();

  fetcher_.StartGetFamilyProfile();

  IssueAccessToken();

  // Invalid response data should result in a service error.
  EXPECT_CALL(*this, OnFailure(FamilyInfoFetcher::SERVICE_ERROR));
  SendInvalidGetFamilyProfileResponse();
}

TEST_F(FamilyInfoFetcherTest, FailedResponse) {
  IssueRefreshToken();

  fetcher_.StartGetFamilyProfile();

  IssueAccessToken();

  // Failed API call should result in a network error.
  EXPECT_CALL(*this, OnFailure(FamilyInfoFetcher::NETWORK_ERROR));
  SendFailedResponse();
}
