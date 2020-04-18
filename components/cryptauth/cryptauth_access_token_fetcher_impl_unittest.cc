// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_access_token_fetcher_impl.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

namespace {

const char kAccountId[] = "account_id";
const char kAccessToken[] = "access_token";
const char kInvalidResult[] = "invalid_result";

// Callback that saves the fetched access token to the first argument.
void SaveAccessToken(std::string* out_token, const std::string& in_token) {
  *out_token = in_token;
}

}  // namespace

class CryptAuthAccessTokenFetcherTest : public testing::Test {
 protected:
  CryptAuthAccessTokenFetcherTest()
      : fetcher_(&token_service_, kAccountId) {
    token_service_.AddAccount(kAccountId);
  }

  FakeOAuth2TokenService token_service_;
  CryptAuthAccessTokenFetcherImpl fetcher_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthAccessTokenFetcherTest);
};

TEST_F(CryptAuthAccessTokenFetcherTest, FetchSuccess) {
  std::string result;
  fetcher_.FetchAccessToken(base::Bind(SaveAccessToken, &result));
  token_service_.IssueAllTokensForAccount(kAccountId, kAccessToken,
                                          base::Time::Max());

  EXPECT_EQ(kAccessToken, result);
}

TEST_F(CryptAuthAccessTokenFetcherTest, FetchFailure) {
  std::string result(kInvalidResult);
  fetcher_.FetchAccessToken(base::Bind(SaveAccessToken, &result));
  token_service_.IssueErrorForAllPendingRequestsForAccount(
      kAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));

  EXPECT_EQ(std::string(), result);
}

TEST_F(CryptAuthAccessTokenFetcherTest, FetcherReuse) {
  std::string result1;
  fetcher_.FetchAccessToken(base::Bind(SaveAccessToken, &result1));

  {
    std::string result2(kInvalidResult);
    fetcher_.FetchAccessToken(base::Bind(SaveAccessToken, &result2));
    EXPECT_EQ(std::string(), result2);
  }

  token_service_.IssueAllTokensForAccount(kAccountId, kAccessToken,
                                          base::Time::Max());
  EXPECT_EQ(kAccessToken, result1);
}

}  // namespace cryptauth
