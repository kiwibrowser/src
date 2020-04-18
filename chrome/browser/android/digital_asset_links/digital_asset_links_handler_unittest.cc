// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/digital_asset_links/digital_asset_links_handler.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "content/public/test/test_browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "services/data_decoder/public/cpp/testing_json_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace digital_asset_links {
namespace {

class DigitalAssetLinksHandlerTest : public ::testing::Test {
 public:
  DigitalAssetLinksHandlerTest()
      : num_invocations_(0),
        result_(RelationshipCheckResult::SUCCESS),
        io_thread_(content::BrowserThread::IO, &message_loop_) {}

  void OnRelationshipCheckComplete(RelationshipCheckResult result) {
    ++num_invocations_;
    result_ = result;
  }

 protected:
  void SetUp() override { num_invocations_ = 0; }

  void SendResponse(net::Error error, int response_code, bool linked) {
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);
    fetcher->set_status(net::URLRequestStatus::FromError(error));
    fetcher->set_response_code(response_code);
    if (error == net::OK && response_code == net::HTTP_OK && linked) {
      fetcher->SetResponseString(
          R"({
            "linked":  true ,
            "maxAge": "40.188652381s"
          })");
    } else if (error == net::OK && response_code == net::HTTP_OK) {
      fetcher->SetResponseString(
          R"({
            "linked":  false ,
            "maxAge": "40.188652381s"
          })");
    } else if (error == net::OK && response_code == net::HTTP_BAD_REQUEST) {
      fetcher->SetResponseString(
          R"({
            "code":  400 ,
            "message": "Invalid statement query received."
            "status": "INVALID_ARGUMENT"
          })");
    } else {
      fetcher->SetResponseString("");
    }
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    base::RunLoop().RunUntilIdle();
  }

  int num_invocations_;
  RelationshipCheckResult result_;

 private:
  base::MessageLoop message_loop_;
  data_decoder::TestingJsonParser::ScopedFactoryOverride factory_override_;
  content::TestBrowserThread io_thread_;
  net::TestURLFetcherFactory url_fetcher_factory_;

  DISALLOW_COPY_AND_ASSIGN(DigitalAssetLinksHandlerTest);
};
}  // namespace

TEST_F(DigitalAssetLinksHandlerTest, PositiveResponse) {
  DigitalAssetLinksHandler handler(nullptr);
  handler.CheckDigitalAssetLinkRelationship(
      base::Bind(&DigitalAssetLinksHandlerTest::OnRelationshipCheckComplete,
                 base::Unretained(this)),
      "", "", "", "");
  SendResponse(net::OK, net::HTTP_OK, true);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_EQ(result_, RelationshipCheckResult::SUCCESS);
}

TEST_F(DigitalAssetLinksHandlerTest, NegativeResponse) {
  DigitalAssetLinksHandler handler(nullptr);
  handler.CheckDigitalAssetLinkRelationship(
      base::Bind(&DigitalAssetLinksHandlerTest::OnRelationshipCheckComplete,
                 base::Unretained(this)),
      "", "", "", "");
  SendResponse(net::OK, net::HTTP_OK, false);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_EQ(result_, RelationshipCheckResult::FAILURE);
}

TEST_F(DigitalAssetLinksHandlerTest, BadRequest) {
  DigitalAssetLinksHandler handler(nullptr);
  handler.CheckDigitalAssetLinkRelationship(
      base::Bind(&DigitalAssetLinksHandlerTest::OnRelationshipCheckComplete,
                 base::Unretained(this)),
      "", "", "", "");
  SendResponse(net::OK, net::HTTP_BAD_REQUEST, true);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_EQ(result_, RelationshipCheckResult::FAILURE);
}

TEST_F(DigitalAssetLinksHandlerTest, NetworkError) {
  DigitalAssetLinksHandler handler(nullptr);
  handler.CheckDigitalAssetLinkRelationship(
      base::Bind(&DigitalAssetLinksHandlerTest::OnRelationshipCheckComplete,
                 base::Unretained(this)),
      "", "", "", "");
  SendResponse(net::ERR_ABORTED, net::HTTP_OK, true);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_EQ(result_, RelationshipCheckResult::FAILURE);
}

TEST_F(DigitalAssetLinksHandlerTest, NetworkDisconnected) {
  DigitalAssetLinksHandler handler(nullptr);
  handler.CheckDigitalAssetLinkRelationship(
      base::Bind(&DigitalAssetLinksHandlerTest::OnRelationshipCheckComplete,
                 base::Unretained(this)),
      "", "", "", "");
  SendResponse(net::ERR_INTERNET_DISCONNECTED, net::HTTP_OK, true);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_EQ(result_, RelationshipCheckResult::NO_CONNECTION);
}
}  // namespace digital_asset_links
