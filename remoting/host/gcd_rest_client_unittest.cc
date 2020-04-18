// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/gcd_rest_client.h"

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/simple_test_clock.h"
#include "base/values.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "remoting/base/fake_oauth_token_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

class GcdRestClientTest : public testing::Test {
 public:
  GcdRestClientTest()
      : default_token_getter_(OAuthTokenGetter::SUCCESS,
                              "<fake_user_email>",
                              "<fake_access_token>") {}

  void OnRequestComplete(GcdRestClient::Result result) {
    ++counter_;
    last_result_ = result;
  }

  std::unique_ptr<base::DictionaryValue> MakePatchDetails(int id) {
    std::unique_ptr<base::DictionaryValue> patch_details(
        new base::DictionaryValue);
    patch_details->SetInteger("id", id);
    return patch_details;
  }

  void CreateClient(OAuthTokenGetter* token_getter = nullptr) {
    if (!token_getter) {
      token_getter = &default_token_getter_;
    }
    client_.reset(new GcdRestClient("http://gcd_base_url", "<gcd_device_id>",
                                    nullptr, token_getter));
    client_->SetClockForTest(&clock_);
  }

 protected:
  net::TestURLFetcherFactory url_fetcher_factory_;
  FakeOAuthTokenGetter default_token_getter_;
  base::SimpleTestClock clock_;
  std::unique_ptr<GcdRestClient> client_;
  int counter_ = 0;
  GcdRestClient::Result last_result_ = GcdRestClient::OTHER_ERROR;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(GcdRestClientTest, NetworkErrorGettingToken) {
  FakeOAuthTokenGetter token_getter(OAuthTokenGetter::NETWORK_ERROR, "", "");
  CreateClient(&token_getter);

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::NETWORK_ERROR, last_result_);
}

TEST_F(GcdRestClientTest, AuthErrorGettingToken) {
  FakeOAuthTokenGetter token_getter(OAuthTokenGetter::AUTH_ERROR, "", "");
  CreateClient(&token_getter);

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::OTHER_ERROR, last_result_);
}

TEST_F(GcdRestClientTest, NetworkErrorOnPost) {
  CreateClient();

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(0);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::NETWORK_ERROR, last_result_);
}

TEST_F(GcdRestClientTest, OtherErrorOnPost) {
  CreateClient();

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(500);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::OTHER_ERROR, last_result_);
}

TEST_F(GcdRestClientTest, NoSuchHost) {
  CreateClient();

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(404);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::NO_SUCH_HOST, last_result_);
}

TEST_F(GcdRestClientTest, Succeed) {
  CreateClient();

  client_->PatchState(MakePatchDetails(0),
                      base::Bind(&GcdRestClientTest::OnRequestComplete,
                                 base::Unretained(this)));
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fetcher);
  EXPECT_EQ("http://gcd_base_url/devices/%3Cgcd_device_id%3E/patchState",
            fetcher->GetOriginalURL().spec());
  EXPECT_EQ(
      "{\"patches\":[{\"patch\":{\"id\":0},\"timeMs\":0.0}],"
      "\"requestTimeMs\":0.0}",
      fetcher->upload_data());
  EXPECT_EQ("application/json", fetcher->upload_content_type());
  fetcher->set_response_code(200);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, counter_);
  EXPECT_EQ(GcdRestClient::SUCCESS, last_result_);
}

}  // namespace remoting
