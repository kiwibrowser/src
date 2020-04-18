// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/http_ice_config_request.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "remoting/base/fake_oauth_token_getter.h"
#include "remoting/base/url_request.h"
#include "remoting/protocol/ice_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {
namespace protocol {

namespace {

const char kTestResponse[] =
    "{"
    "  \"lifetimeDuration\": \"43200.000s\","
    "  \"iceServers\": ["
    "    {"
    "      \"urls\": ["
    "        \"turns:the_server.com\""
    "      ],"
    "      \"username\": \"123\","
    "      \"credential\": \"abc\""
    "    },"
    "    {"
    "      \"urls\": ["
    "        \"stun:stun_server.com:18344\""
    "      ]"
    "    }"
    "  ]"
    "}";
const char kTestOAuthToken[] = "TestOAuthToken";

class FakeUrlRequest : public UrlRequest {
 public:
  FakeUrlRequest(const Result& result, bool expect_oauth_token)
      : result_(result), expect_oauth_token_(expect_oauth_token) {}
  ~FakeUrlRequest() override = default;

  // UrlRequest interface.
  void AddHeader(const std::string& value) override {
    EXPECT_TRUE(expect_oauth_token_);
    EXPECT_EQ(value, std::string("Authorization:Bearer ") + kTestOAuthToken);
    expect_oauth_token_ = false;
  }

  void SetPostData(const std::string& content_type,
                   const std::string& post_data) override {
    EXPECT_EQ("application/json", content_type);
    EXPECT_EQ("", post_data);
  }

  void Start(const OnResultCallback& on_result_callback) override {
    EXPECT_FALSE(expect_oauth_token_);
    on_result_callback.Run(result_);
  }

 private:
  Result result_;
  bool expect_oauth_token_;
};

class FakeUrlRequestFactory : public UrlRequestFactory {
 public:
  FakeUrlRequestFactory() = default;
  ~FakeUrlRequestFactory() override = default;

  void SetResult(const std::string& url, const UrlRequest::Result& result) {
    results_[url] = result;
  }

  void set_expect_oauth_token(bool expect_oauth_token) {
    expect_oauth_token_ = expect_oauth_token;
  }

  // UrlRequestFactory interface.
  std::unique_ptr<UrlRequest> CreateUrlRequest(
      UrlRequest::Type type,
      const std::string& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override {
    EXPECT_EQ(UrlRequest::Type::GET, type);
    EXPECT_TRUE(results_.count(url));
    return std::make_unique<FakeUrlRequest>(results_[url], expect_oauth_token_);
  }

 private:
  std::map<std::string, UrlRequest::Result> results_;

  bool expect_oauth_token_ = false;
};

}  // namespace

static const char kTestUrl[] = "http://host/ice_config";

class HttpIceConfigRequestTest : public testing::Test {
 public:
  void OnResult(const IceConfig& config) {
    received_config_ = std::make_unique<IceConfig>(config);
  }

 protected:
  base::MessageLoop message_loop_;
  FakeUrlRequestFactory url_request_factory_;
  std::unique_ptr<HttpIceConfigRequest> request_;
  std::unique_ptr<IceConfig> received_config_;
};

TEST_F(HttpIceConfigRequestTest, Parse) {
  url_request_factory_.SetResult(kTestUrl,
                                 UrlRequest::Result(200, kTestResponse));
  request_.reset(
      new HttpIceConfigRequest(&url_request_factory_, kTestUrl, nullptr));
  request_->Send(
      base::Bind(&HttpIceConfigRequestTest::OnResult, base::Unretained(this)));
  ASSERT_FALSE(received_config_->is_null());

  EXPECT_EQ(1U, received_config_->turn_servers.size());
  EXPECT_EQ(1U, received_config_->stun_servers.size());
}

TEST_F(HttpIceConfigRequestTest, InvalidConfig) {
  url_request_factory_.SetResult(kTestUrl,
                                 UrlRequest::Result(200, "ERROR"));
  request_.reset(
      new HttpIceConfigRequest(&url_request_factory_, kTestUrl, nullptr));
  request_->Send(
      base::Bind(&HttpIceConfigRequestTest::OnResult, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(received_config_->is_null());
}

TEST_F(HttpIceConfigRequestTest, FailedRequest) {
  url_request_factory_.SetResult(kTestUrl, UrlRequest::Result::Failed());
  request_.reset(
      new HttpIceConfigRequest(&url_request_factory_, kTestUrl, nullptr));
  request_->Send(
      base::Bind(&HttpIceConfigRequestTest::OnResult, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(received_config_->is_null());
}

TEST_F(HttpIceConfigRequestTest, Authentication) {
  url_request_factory_.SetResult(kTestUrl,
                                 UrlRequest::Result(200, kTestResponse));
  url_request_factory_.set_expect_oauth_token(true);

  FakeOAuthTokenGetter token_getter(OAuthTokenGetter::SUCCESS,
                                    "user@example.com", kTestOAuthToken);
  request_ = std::make_unique<HttpIceConfigRequest>(&url_request_factory_,
                                                    kTestUrl, &token_getter);
  request_->Send(
      base::Bind(&HttpIceConfigRequestTest::OnResult, base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(received_config_);

  EXPECT_EQ(1U, received_config_->turn_servers.size());
  EXPECT_EQ(1U, received_config_->stun_servers.size());
}

}  // namespace protocol
}  // namespace remoting
