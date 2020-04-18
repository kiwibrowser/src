// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/dom_distiller/core/distiller_url_fetcher.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

const char kTestPageA[] = "http://www.a.com/";
const char kTestPageAResponse[] = { 1, 2, 3, 4, 5, 6, 7 };
const char kTestPageB[] = "http://www.b.com/";
const char kTestPageBResponse[] = { 'a', 'b', 'c' };


class DistillerURLFetcherTest : public testing::Test {
 public:
  void FetcherCallback(const std::string& response) {
     response_ = response;
  }

 protected:
  // testing::Test implementation:
  void SetUp() override {
    url_fetcher_.reset(new dom_distiller::DistillerURLFetcher(nullptr));
    factory_.reset(new net::FakeURLFetcherFactory(nullptr));
    factory_->SetFakeResponse(
        GURL(kTestPageA),
        std::string(kTestPageAResponse, sizeof(kTestPageAResponse)),
        net::HTTP_OK,
        net::URLRequestStatus::SUCCESS);
    factory_->SetFakeResponse(
        GURL(kTestPageB),
        std::string(kTestPageBResponse, sizeof(kTestPageBResponse)),
        net::HTTP_INTERNAL_SERVER_ERROR,
        net::URLRequestStatus::SUCCESS);
  }

  void Fetch(const std::string& url,
             const std::string& expected_response) {
    base::MessageLoopForUI loop;
    url_fetcher_->FetchURL(
        url,
        base::Bind(&DistillerURLFetcherTest::FetcherCallback,
                   base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
    CHECK_EQ(expected_response, response_);
  }

  std::unique_ptr<dom_distiller::DistillerURLFetcher> url_fetcher_;
  std::unique_ptr<net::FakeURLFetcherFactory> factory_;
  std::string response_;
};

TEST_F(DistillerURLFetcherTest, PopulateProto) {
  Fetch(kTestPageA,
        std::string(kTestPageAResponse, sizeof(kTestPageAResponse)));
}

TEST_F(DistillerURLFetcherTest, PopulateProtoFailedFetch) {
  // Expect the callback to contain an empty string for this URL.
  Fetch(kTestPageB, std::string(std::string(), 0));
}
