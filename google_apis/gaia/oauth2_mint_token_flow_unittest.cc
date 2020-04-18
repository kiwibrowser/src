// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A complete set of unit tests for OAuth2MintTokenFlow.

#include "google_apis/gaia/oauth2_mint_token_flow.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_access_token_fetcher.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::TestURLFetcher;
using net::URLFetcher;
using net::URLRequestStatus;
using testing::_;
using testing::StrictMock;

namespace {

static const char kValidTokenResponse[] =
    "{"
    "  \"token\": \"at1\","
    "  \"issueAdvice\": \"Auto\","
    "  \"expiresIn\": \"3600\""
    "}";
static const char kTokenResponseNoAccessToken[] =
    "{"
    "  \"issueAdvice\": \"Auto\""
    "}";

static const char kValidIssueAdviceResponse[] =
    "{"
    "  \"issueAdvice\": \"consent\","
    "  \"consent\": {"
    "    \"oauthClient\": {"
    "      \"name\": \"Test app\","
    "      \"iconUri\": \"\","
    "      \"developerEmail\": \"munjal@chromium.org\""
    "    },"
    "    \"scopes\": ["
    "      {"
    "        \"description\": \"Manage your calendars\","
    "        \"detail\": \"\nView and manage your calendars\n\""
    "      },"
    "      {"
    "        \"description\": \"Manage your documents\","
    "        \"detail\": \"\nView your documents\nUpload new documents\n\""
    "      }"
    "    ]"
    "  }"
    "}";

static const char kIssueAdviceResponseNoDescription[] =
    "{"
    "  \"issueAdvice\": \"consent\","
    "  \"consent\": {"
    "    \"oauthClient\": {"
    "      \"name\": \"Test app\","
    "      \"iconUri\": \"\","
    "      \"developerEmail\": \"munjal@chromium.org\""
    "    },"
    "    \"scopes\": ["
    "      {"
    "        \"description\": \"Manage your calendars\","
    "        \"detail\": \"\nView and manage your calendars\n\""
    "      },"
    "      {"
    "        \"detail\": \"\nView your documents\nUpload new documents\n\""
    "      }"
    "    ]"
    "  }"
    "}";

static const char kIssueAdviceResponseNoDetail[] =
    "{"
    "  \"issueAdvice\": \"consent\","
    "  \"consent\": {"
    "    \"oauthClient\": {"
    "      \"name\": \"Test app\","
    "      \"iconUri\": \"\","
    "      \"developerEmail\": \"munjal@chromium.org\""
    "    },"
    "    \"scopes\": ["
    "      {"
    "        \"description\": \"Manage your calendars\","
    "        \"detail\": \"\nView and manage your calendars\n\""
    "      },"
    "      {"
    "        \"description\": \"Manage your documents\""
    "      }"
    "    ]"
    "  }"
    "}";

std::vector<std::string> CreateTestScopes() {
  std::vector<std::string> scopes;
  scopes.push_back("http://scope1");
  scopes.push_back("http://scope2");
  return scopes;
}

static IssueAdviceInfo CreateIssueAdvice() {
  IssueAdviceInfo ia;
  IssueAdviceInfoEntry e1;
  e1.description = base::ASCIIToUTF16("Manage your calendars");
  e1.details.push_back(base::ASCIIToUTF16("View and manage your calendars"));
  ia.push_back(e1);
  IssueAdviceInfoEntry e2;
  e2.description = base::ASCIIToUTF16("Manage your documents");
  e2.details.push_back(base::ASCIIToUTF16("View your documents"));
  e2.details.push_back(base::ASCIIToUTF16("Upload new documents"));
  ia.push_back(e2);
  return ia;
}

class MockDelegate : public OAuth2MintTokenFlow::Delegate {
 public:
  MockDelegate() {}
  ~MockDelegate() override {}

  MOCK_METHOD2(OnMintTokenSuccess, void(const std::string& access_token,
                                        int time_to_live));
  MOCK_METHOD1(OnIssueAdviceSuccess,
               void (const IssueAdviceInfo& issue_advice));
  MOCK_METHOD1(OnMintTokenFailure,
               void(const GoogleServiceAuthError& error));
};

class MockMintTokenFlow : public OAuth2MintTokenFlow {
 public:
  explicit MockMintTokenFlow(MockDelegate* delegate,
                             const OAuth2MintTokenFlow::Parameters& parameters)
      : OAuth2MintTokenFlow(delegate, parameters) {}
  ~MockMintTokenFlow() override {}

  MOCK_METHOD0(CreateAccessTokenFetcher, OAuth2AccessTokenFetcher*());
};

}  // namespace

class OAuth2MintTokenFlowTest : public testing::Test {
 public:
  OAuth2MintTokenFlowTest() {}
  ~OAuth2MintTokenFlowTest() override {}

 protected:
  void CreateFlow(OAuth2MintTokenFlow::Mode mode) {
    return CreateFlow(&delegate_, mode, "");
  }

  void CreateFlowWithDeviceId(const std::string& device_id) {
    return CreateFlow(&delegate_, OAuth2MintTokenFlow::MODE_ISSUE_ADVICE,
                      device_id);
  }

  void CreateFlow(MockDelegate* delegate,
                  OAuth2MintTokenFlow::Mode mode,
                  const std::string& device_id) {
    std::string ext_id = "ext1";
    std::string client_id = "client1";
    std::vector<std::string> scopes(CreateTestScopes());
    flow_.reset(new MockMintTokenFlow(
        delegate, OAuth2MintTokenFlow::Parameters(ext_id, client_id, scopes,
                                                  device_id, mode)));
  }

  // Helper to parse the given string to DictionaryValue.
  static base::DictionaryValue* ParseJson(const std::string& str) {
    std::unique_ptr<base::Value> value = base::JSONReader::Read(str);
    EXPECT_TRUE(value.get());
    EXPECT_EQ(base::Value::Type::DICTIONARY, value->type());
    return static_cast<base::DictionaryValue*>(value.release());
  }

  std::unique_ptr<MockMintTokenFlow> flow_;
  StrictMock<MockDelegate> delegate_;
};

TEST_F(OAuth2MintTokenFlowTest, CreateApiCallBody) {
  {  // Issue advice mode.
    CreateFlow(OAuth2MintTokenFlow::MODE_ISSUE_ADVICE);
    std::string body = flow_->CreateApiCallBody();
    std::string expected_body(
        "force=false"
        "&response_type=none"
        "&scope=http://scope1+http://scope2"
        "&client_id=client1"
        "&origin=ext1");
    EXPECT_EQ(expected_body, body);
  }
  {  // Record grant mode.
    CreateFlow(OAuth2MintTokenFlow::MODE_RECORD_GRANT);
    std::string body = flow_->CreateApiCallBody();
    std::string expected_body(
        "force=true"
        "&response_type=none"
        "&scope=http://scope1+http://scope2"
        "&client_id=client1"
        "&origin=ext1");
    EXPECT_EQ(expected_body, body);
  }
  {  // Mint token no force mode.
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    std::string body = flow_->CreateApiCallBody();
    std::string expected_body(
        "force=false"
        "&response_type=token"
        "&scope=http://scope1+http://scope2"
        "&client_id=client1"
        "&origin=ext1");
    EXPECT_EQ(expected_body, body);
  }
  {  // Mint token force mode.
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_FORCE);
    std::string body = flow_->CreateApiCallBody();
    std::string expected_body(
        "force=true"
        "&response_type=token"
        "&scope=http://scope1+http://scope2"
        "&client_id=client1"
        "&origin=ext1");
    EXPECT_EQ(expected_body, body);
  }
  {  // Mint token with device_id.
    CreateFlowWithDeviceId("device_id1");
    std::string body = flow_->CreateApiCallBody();
    std::string expected_body(
        "force=false"
        "&response_type=none"
        "&scope=http://scope1+http://scope2"
        "&client_id=client1"
        "&origin=ext1"
        "&device_id=device_id1"
        "&device_type=chrome"
        "&lib_ver=extension");
    EXPECT_EQ(expected_body, body);
  }
}

TEST_F(OAuth2MintTokenFlowTest, ParseMintTokenResponse) {
  {  // Access token missing.
    std::unique_ptr<base::DictionaryValue> json(
        ParseJson(kTokenResponseNoAccessToken));
    std::string at;
    int ttl;
    EXPECT_FALSE(OAuth2MintTokenFlow::ParseMintTokenResponse(json.get(), &at,
                                                             &ttl));
    EXPECT_TRUE(at.empty());
  }
  {  // All good.
    std::unique_ptr<base::DictionaryValue> json(ParseJson(kValidTokenResponse));
    std::string at;
    int ttl;
    EXPECT_TRUE(OAuth2MintTokenFlow::ParseMintTokenResponse(json.get(), &at,
                                                            &ttl));
    EXPECT_EQ("at1", at);
    EXPECT_EQ(3600, ttl);
  }
}

TEST_F(OAuth2MintTokenFlowTest, ParseIssueAdviceResponse) {
  {  // Description missing.
    std::unique_ptr<base::DictionaryValue> json(
        ParseJson(kIssueAdviceResponseNoDescription));
    IssueAdviceInfo ia;
    EXPECT_FALSE(OAuth2MintTokenFlow::ParseIssueAdviceResponse(
        json.get(), &ia));
    EXPECT_TRUE(ia.empty());
  }
  {  // Detail missing.
    std::unique_ptr<base::DictionaryValue> json(
        ParseJson(kIssueAdviceResponseNoDetail));
    IssueAdviceInfo ia;
    EXPECT_FALSE(OAuth2MintTokenFlow::ParseIssueAdviceResponse(
        json.get(), &ia));
    EXPECT_TRUE(ia.empty());
  }
  {  // All good.
    std::unique_ptr<base::DictionaryValue> json(
        ParseJson(kValidIssueAdviceResponse));
    IssueAdviceInfo ia;
    EXPECT_TRUE(OAuth2MintTokenFlow::ParseIssueAdviceResponse(
        json.get(), &ia));
    IssueAdviceInfo ia_expected(CreateIssueAdvice());
    EXPECT_EQ(ia_expected, ia);
  }
}

TEST_F(OAuth2MintTokenFlowTest, ProcessApiCallSuccess) {
  {  // No body.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(std::string());
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Bad json.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString("foo");
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Valid json: no access token.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(kTokenResponseNoAccessToken);
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Valid json: good token response.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(kValidTokenResponse);
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    EXPECT_CALL(delegate_, OnMintTokenSuccess("at1", 3600));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Valid json: no description.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(kIssueAdviceResponseNoDescription);
    CreateFlow(OAuth2MintTokenFlow::MODE_ISSUE_ADVICE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Valid json: no detail.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(kIssueAdviceResponseNoDetail);
    CreateFlow(OAuth2MintTokenFlow::MODE_ISSUE_ADVICE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
  {  // Valid json: good issue advice response.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.SetResponseString(kValidIssueAdviceResponse);
    CreateFlow(OAuth2MintTokenFlow::MODE_ISSUE_ADVICE);
    IssueAdviceInfo ia(CreateIssueAdvice());
    EXPECT_CALL(delegate_, OnIssueAdviceSuccess(ia));
    flow_->ProcessApiCallSuccess(&url_fetcher);
  }
}

TEST_F(OAuth2MintTokenFlowTest, ProcessApiCallFailure) {
  {  // Null delegate should work fine.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.set_status(URLRequestStatus::FromError(net::ERR_FAILED));
    CreateFlow(NULL, OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE, "");
    flow_->ProcessApiCallFailure(&url_fetcher);
  }

  {  // Non-null delegate.
    TestURLFetcher url_fetcher(1, GURL("http://www.google.com"), NULL);
    url_fetcher.set_status(URLRequestStatus::FromError(net::ERR_FAILED));
    CreateFlow(OAuth2MintTokenFlow::MODE_MINT_TOKEN_NO_FORCE);
    EXPECT_CALL(delegate_, OnMintTokenFailure(_));
    flow_->ProcessApiCallFailure(&url_fetcher);
  }
}
