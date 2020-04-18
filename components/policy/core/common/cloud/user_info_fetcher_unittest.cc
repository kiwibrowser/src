// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "components/policy/core/common/cloud/user_info_fetcher.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace policy {

namespace {

static const char kUserInfoResponse[] =
    "{"
    "  \"email\": \"test_user@test.com\","
    "  \"verified_email\": true,"
    "  \"hd\": \"test.com\""
    "}";

class MockUserInfoFetcherDelegate : public UserInfoFetcher::Delegate {
 public:
  MockUserInfoFetcherDelegate() {}
  ~MockUserInfoFetcherDelegate() {}
  MOCK_METHOD1(OnGetUserInfoFailure,
               void(const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnGetUserInfoSuccess, void(const base::DictionaryValue* result));
};

MATCHER_P(MatchDict, expected, "matches DictionaryValue") {
  return arg->Equals(expected);
}

class UserInfoFetcherTest : public testing::Test {
 public:
  UserInfoFetcherTest() {}
  net::TestURLFetcherFactory url_factory_;
};

TEST_F(UserInfoFetcherTest, FailedFetch) {
  MockUserInfoFetcherDelegate delegate;
  UserInfoFetcher fetcher(&delegate, nullptr);
  fetcher.Start("access_token");

  // Fake a failed fetch - should result in the failure callback being invoked.
  EXPECT_CALL(delegate, OnGetUserInfoFailure(_));
  net::TestURLFetcher* url_fetcher = url_factory_.GetFetcherByID(0);
  url_fetcher->set_status(net::URLRequestStatus::FromError(net::ERR_FAILED));
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}

TEST_F(UserInfoFetcherTest, SuccessfulFetch) {
  MockUserInfoFetcherDelegate delegate;
  UserInfoFetcher fetcher(&delegate, nullptr);
  fetcher.Start("access_token");

  // Generate what we expect our result will look like (should match
  // parsed kUserInfoResponse).
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetString("email", "test_user@test.com");
  dict->SetBoolean("verified_email", true);
  dict->SetString("hd", "test.com");

  // Fake a successful fetch - should result in the data being parsed and
  // the values passed off to the success callback.
  EXPECT_CALL(delegate, OnGetUserInfoSuccess(MatchDict(dict.get())));
  net::TestURLFetcher* url_fetcher = url_factory_.GetFetcherByID(0);
  url_fetcher->set_response_code(net::HTTP_OK);
  url_fetcher->SetResponseString(kUserInfoResponse);
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}
}  // namespace

}  // namespace policy
