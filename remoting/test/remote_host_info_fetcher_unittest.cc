// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/remote_host_info_fetcher.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kTestApplicationId[] = "klasdfjlkasdfjklasjfdkljsadf";
const char kTestApplicationId2[] = "klasdfjlkasdfjklasjfdkljsadf2";
const char kAccessTokenValue[] = "test_access_token_value";
const char kRemoteHostInfoReadyResponse[] =
    "{"
    "  \"status\": \"done\","
    "  \"host\": {"
    "    \"kind\": \"test_kind\","
    "    \"applicationId\": \"klasdfjlkasdfjklasjfdkljsadf\","
    "    \"hostId\": \"test_host_id\""
    "  },"
    "  \"hostJid\": \"test_host_jid\","
    "  \"authorizationCode\": \"test_authorization_code\","
    "  \"sharedSecret\": \"test_shared_secret\""
    "}";
const char kRemoteHostInfoReadyResponse2[] =
    "{"
    "  \"status\": \"done\","
    "  \"host\": {"
    "    \"kind\": \"test_kind\","
    "    \"applicationId\": \"klasdfjlkasdfjklasjfdkljsadf2\","
    "    \"hostId\": \"test_host_id\""
    "  },"
    "  \"hostJid\": \"test_host_jid\","
    "  \"authorizationCode\": \"test_authorization_code\","
    "  \"sharedSecret\": \"test_shared_secret\""
    "}";
const char kRemoteHostInfoPendingResponse[] =
    "{"
    "  \"status\": \"pending\""
    "}";
const char kRemoteHostInfoEmptyResponse[] = "{}";
}  // namespace

namespace remoting {
namespace test {

// Provides base functionality for the RemoteHostInfoFetcher Tests below.  The
// FakeURLFetcherFactory allows us to override the response data and payload for
// specified URLs.  We use this to stub out network calls made by the
// RemoteHostInfoFetcher.  This fixture also creates an IO MessageLoop, if
// necessary, for use by the RemoteHostInfoFetcher.
class RemoteHostInfoFetcherTest : public ::testing::Test {
 public:
  RemoteHostInfoFetcherTest() : url_fetcher_factory_(nullptr) {}
  ~RemoteHostInfoFetcherTest() override = default;

  // Used as a RemoteHostInfoCallback for testing.
  void OnRemoteHostInfoRetrieved(
      base::Closure done_closure,
      const RemoteHostInfo& retrieved_remote_host_info);

 protected:
  // testing::Test interface.
  void SetUp() override;

  // Sets the HTTP status and data returned for a specified URL.
  void SetFakeResponse(const GURL& url,
                       const std::string& data,
                       net::HttpStatusCode code,
                       net::URLRequestStatus::Status status);

  // Used for result verification.
  RemoteHostInfo remote_host_info_;

  std::string dev_service_environment_url_;
  std::string dev_service_environment_url_2_;

 private:
  net::FakeURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<base::MessageLoopForIO> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(RemoteHostInfoFetcherTest);
};

void RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved(
    base::Closure done_closure,
    const RemoteHostInfo& retrieved_remote_host_info) {
  remote_host_info_ = retrieved_remote_host_info;

  done_closure.Run();
}

void RemoteHostInfoFetcherTest::SetUp() {
  DCHECK(!message_loop_);
  message_loop_.reset(new base::MessageLoopForIO);

  dev_service_environment_url_ =
      GetRunApplicationUrl(kTestApplicationId, kDeveloperEnvironment);
  SetFakeResponse(GURL(dev_service_environment_url_),
                  kRemoteHostInfoEmptyResponse, net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::FAILED);

  dev_service_environment_url_2_ =
      GetRunApplicationUrl(kTestApplicationId2, kDeveloperEnvironment);
  SetFakeResponse(GURL(dev_service_environment_url_2_),
                  kRemoteHostInfoEmptyResponse, net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::FAILED);
}

void RemoteHostInfoFetcherTest::SetFakeResponse(
    const GURL& url,
    const std::string& data,
    net::HttpStatusCode code,
    net::URLRequestStatus::Status status) {
  url_fetcher_factory_.SetFakeResponse(url, data, code, status);
}

TEST_F(RemoteHostInfoFetcherTest, RetrieveRemoteHostInfoFromDev) {
  SetFakeResponse(GURL(dev_service_environment_url_),
                  kRemoteHostInfoReadyResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  RemoteHostInfoCallback remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kDeveloperEnvironment,
      remote_host_info_callback);

  run_loop.Run();

  EXPECT_TRUE(request_started);
  EXPECT_TRUE(remote_host_info_.IsReadyForConnection());
  EXPECT_EQ(remote_host_info_.application_id.compare(kTestApplicationId), 0);
  EXPECT_TRUE(!remote_host_info_.host_id.empty());
  EXPECT_TRUE(!remote_host_info_.host_jid.empty());
  EXPECT_TRUE(!remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(!remote_host_info_.shared_secret.empty());
}

TEST_F(RemoteHostInfoFetcherTest, RetrieveRemoteHostInfoInvalidEnvironment) {
  base::RunLoop run_loop;
  RemoteHostInfoCallback remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kUnknownEnvironment,
      remote_host_info_callback);

  EXPECT_FALSE(request_started);
}

TEST_F(RemoteHostInfoFetcherTest, RetrieveRemoteHostInfoNetworkError) {
  base::RunLoop run_loop;
  RemoteHostInfoCallback remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kDeveloperEnvironment,
      remote_host_info_callback);

  run_loop.Run();

  EXPECT_TRUE(request_started);
  EXPECT_FALSE(remote_host_info_.IsReadyForConnection());

  // If there was a network error retrieving the remote host info, then none of
  // the connection details should be populated.
  EXPECT_TRUE(remote_host_info_.application_id.empty());
  EXPECT_TRUE(remote_host_info_.host_id.empty());
  EXPECT_TRUE(remote_host_info_.host_jid.empty());
  EXPECT_TRUE(remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(remote_host_info_.shared_secret.empty());
}

TEST_F(RemoteHostInfoFetcherTest, RetrieveRemoteHostInfoPendingResponse) {
  SetFakeResponse(GURL(dev_service_environment_url_),
                  kRemoteHostInfoPendingResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  RemoteHostInfoCallback remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kDeveloperEnvironment,
      remote_host_info_callback);

  run_loop.Run();

  EXPECT_TRUE(request_started);
  EXPECT_FALSE(remote_host_info_.IsReadyForConnection());

  // If the remote host request is pending, then none of the connection details
  // should be populated.
  EXPECT_TRUE(remote_host_info_.application_id.empty());
  EXPECT_TRUE(remote_host_info_.host_id.empty());
  EXPECT_TRUE(remote_host_info_.host_jid.empty());
  EXPECT_TRUE(remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(remote_host_info_.shared_secret.empty());
}

TEST_F(RemoteHostInfoFetcherTest, RetrieveRemoteHostInfoEmptyResponse) {
  SetFakeResponse(GURL(dev_service_environment_url_),
                  kRemoteHostInfoEmptyResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop run_loop;
  RemoteHostInfoCallback remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kDeveloperEnvironment,
      remote_host_info_callback);

  run_loop.Run();

  EXPECT_TRUE(request_started);
  EXPECT_FALSE(remote_host_info_.IsReadyForConnection());

  // If we received an empty response, then none of the connection details
  // should be populated.
  EXPECT_TRUE(remote_host_info_.application_id.empty());
  EXPECT_TRUE(remote_host_info_.host_id.empty());
  EXPECT_TRUE(remote_host_info_.host_jid.empty());
  EXPECT_TRUE(remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(remote_host_info_.shared_secret.empty());
}

TEST_F(RemoteHostInfoFetcherTest, MultipleRetrieveRemoteHostInfoRequests) {
  // First, we will fetch info from the development service environment.
  SetFakeResponse(GURL(dev_service_environment_url_),
                  kRemoteHostInfoReadyResponse, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop dev_run_loop;
  RemoteHostInfoCallback dev_remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), dev_run_loop.QuitClosure());

  RemoteHostInfoFetcher remote_host_info_fetcher;
  bool dev_request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId, kAccessTokenValue, kDeveloperEnvironment,
      dev_remote_host_info_callback);

  dev_run_loop.Run();

  EXPECT_TRUE(dev_request_started);
  EXPECT_TRUE(remote_host_info_.IsReadyForConnection());
  EXPECT_EQ(remote_host_info_.application_id.compare(kTestApplicationId), 0);
  EXPECT_TRUE(!remote_host_info_.host_id.empty());
  EXPECT_TRUE(!remote_host_info_.host_jid.empty());
  EXPECT_TRUE(!remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(!remote_host_info_.shared_secret.empty());

  // Next, we will fetch a different application info block from the dev
  // service environment.
  SetFakeResponse(GURL(dev_service_environment_url_2_),
                  kRemoteHostInfoReadyResponse2, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);

  base::RunLoop test_run_loop;
  RemoteHostInfoCallback test_remote_host_info_callback =
      base::Bind(&RemoteHostInfoFetcherTest::OnRemoteHostInfoRetrieved,
                 base::Unretained(this), test_run_loop.QuitClosure());

  // Reset the state of our internal |remote_host_info_| object.
  remote_host_info_ = RemoteHostInfo();
  EXPECT_FALSE(remote_host_info_.IsReadyForConnection());

  bool test_request_started = remote_host_info_fetcher.RetrieveRemoteHostInfo(
      kTestApplicationId2, kAccessTokenValue, kDeveloperEnvironment,
      test_remote_host_info_callback);

  test_run_loop.Run();

  EXPECT_TRUE(test_request_started);
  EXPECT_TRUE(remote_host_info_.IsReadyForConnection());
  EXPECT_EQ(remote_host_info_.application_id.compare(kTestApplicationId2), 0);
  EXPECT_TRUE(!remote_host_info_.host_id.empty());
  EXPECT_TRUE(!remote_host_info_.host_jid.empty());
  EXPECT_TRUE(!remote_host_info_.authorization_code.empty());
  EXPECT_TRUE(!remote_host_info_.shared_secret.empty());
}

}  // namespace test
}  // namespace remoting
