// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/gcm_network_channel.h"

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class TestGCMNetworkChannelDelegate : public GCMNetworkChannelDelegate {
 public:
  TestGCMNetworkChannelDelegate()
      : register_call_count_(0) {}

  void Initialize(ConnectionStateCallback connection_state_callback,
                  base::Closure store_reset_callback) override {
    this->connection_state_callback = connection_state_callback;
  }

  void RequestToken(RequestTokenCallback callback) override {
    request_token_callback = callback;
  }

  void InvalidateToken(const std::string& token) override {
    invalidated_token = token;
  }

  void Register(RegisterCallback callback) override {
    ++register_call_count_;
    register_callback = callback;
  }

  void SetMessageReceiver(MessageCallback callback) override {
    message_callback = callback;
  }

  RequestTokenCallback request_token_callback;
  std::string invalidated_token;
  RegisterCallback register_callback;
  int register_call_count_;
  MessageCallback message_callback;
  ConnectionStateCallback connection_state_callback;
};

// Backoff policy for test. Run first 5 retries without delay.
const net::BackoffEntry::Policy kTestBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  5,

  // Initial delay for exponential back-off in ms.
  2000, // 2 seconds.

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.2, // 20%.

  // Maximum amount of time we are willing to delay our request in ms.
  1000 * 3600 * 4, // 4 hours.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

class TestGCMNetworkChannel : public GCMNetworkChannel {
 public:
  TestGCMNetworkChannel(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      std::unique_ptr<GCMNetworkChannelDelegate> delegate)
      : GCMNetworkChannel(request_context_getter, std::move(delegate)) {
    ResetRegisterBackoffEntryForTest(&kTestBackoffPolicy);
  }

 protected:
  // On Android GCMNetworkChannel::BuildUrl hits NOTREACHED(). I still want
  // tests to run.
  GURL BuildUrl(const std::string& registration_id) override {
    return GURL("http://test.url.com");
  }
};

class GCMNetworkChannelTest;

// Test needs to capture setting echo-token header on http request.
// This class is going to do that.
class TestNetworkChannelURLFetcher : public net::FakeURLFetcher {
 public:
  TestNetworkChannelURLFetcher(GCMNetworkChannelTest* test,
                               const GURL& url,
                               net::URLFetcherDelegate* delegate,
                               const std::string& response_data,
                               net::HttpStatusCode response_code,
                               net::URLRequestStatus::Status status)
      : net::FakeURLFetcher(url,
                            delegate,
                            response_data,
                            response_code,
                            status),
        test_(test) {}

  void AddExtraRequestHeader(const std::string& header_line) override;

 private:
  GCMNetworkChannelTest* test_;
};

class GCMNetworkChannelTest
    : public ::testing::Test,
      public SyncNetworkChannel::Observer {
 public:
  GCMNetworkChannelTest()
      : delegate_(nullptr),
        url_fetchers_created_count_(0),
        last_invalidator_state_(TRANSIENT_INVALIDATION_ERROR) {}

  ~GCMNetworkChannelTest() override {}

  void SetUp() override {
    request_context_getter_ = new net::TestURLRequestContextGetter(
        base::ThreadTaskRunnerHandle::Get());
    // Ownership of delegate goes to GCNMentworkChannel but test needs pointer
    // to it.
    delegate_ = new TestGCMNetworkChannelDelegate();
    std::unique_ptr<GCMNetworkChannelDelegate> delegate(delegate_);
    gcm_network_channel_.reset(new TestGCMNetworkChannel(
        request_context_getter_, std::move(delegate)));
    gcm_network_channel_->AddObserver(this);
    gcm_network_channel_->SetMessageReceiver(
        invalidation::NewPermanentCallback(
            this, &GCMNetworkChannelTest::OnIncomingMessage));
    url_fetcher_factory_.reset(new net::FakeURLFetcherFactory(
        nullptr, base::Bind(&GCMNetworkChannelTest::CreateURLFetcher,
                            base::Unretained(this))));
  }

  void TearDown() override { gcm_network_channel_->RemoveObserver(this); }

  // Helper functions to call private methods from test
  GURL BuildUrl(const std::string& registration_id) {
    return gcm_network_channel_->GCMNetworkChannel::BuildUrl(registration_id);
  }

  void OnNetworkChannelStateChanged(
      InvalidatorState invalidator_state) override {
    last_invalidator_state_ = invalidator_state;
  }

  void OnIncomingMessage(std::string /* incoming_message */) {}

  GCMNetworkChannel* network_channel() {
    return gcm_network_channel_.get();
  }

  TestGCMNetworkChannelDelegate* delegate() {
    return delegate_;
  }

  int url_fetchers_created_count() {
    return url_fetchers_created_count_;
  }

  net::FakeURLFetcherFactory* url_fetcher_factory() {
    return url_fetcher_factory_.get();
  }

  std::unique_ptr<net::FakeURLFetcher> CreateURLFetcher(
      const GURL& url,
      net::URLFetcherDelegate* delegate,
      const std::string& response_data,
      net::HttpStatusCode response_code,
      net::URLRequestStatus::Status status) {
    ++url_fetchers_created_count_;
    return std::unique_ptr<net::FakeURLFetcher>(
        new TestNetworkChannelURLFetcher(this, url, delegate, response_data,
                                         response_code, status));
  }

  void set_last_echo_token(const std::string& echo_token) {
    last_echo_token_ = echo_token;
  }

  const std::string& get_last_echo_token() {
    return last_echo_token_;
  }

  InvalidatorState get_last_invalidator_state() {
    return last_invalidator_state_;
  }

  void RunLoopUntilIdle() {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestGCMNetworkChannelDelegate* delegate_;
  std::unique_ptr<GCMNetworkChannel> gcm_network_channel_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  std::unique_ptr<net::FakeURLFetcherFactory> url_fetcher_factory_;
  int url_fetchers_created_count_;
  std::string last_echo_token_;
  InvalidatorState last_invalidator_state_;
};

void TestNetworkChannelURLFetcher::AddExtraRequestHeader(
    const std::string& header_line) {
  net::FakeURLFetcher::AddExtraRequestHeader(header_line);
  std::string header_name("echo-token: ");
  std::string echo_token;
  if (base::StartsWith(header_line, header_name,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    echo_token = header_line;
    base::ReplaceFirstSubstringAfterOffset(
        &echo_token, 0, header_name, std::string());
    test_->set_last_echo_token(echo_token);
  }
}

TEST_F(GCMNetworkChannelTest, HappyCase) {
  EXPECT_EQ(TRANSIENT_INVALIDATION_ERROR, get_last_invalidator_state());
  EXPECT_FALSE(delegate()->message_callback.is_null());
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         std::string(),
                                         net::HTTP_NO_CONTENT,
                                         net::URLRequestStatus::SUCCESS);

  // Emulate gcm connection state to be online.
  delegate()->connection_state_callback.Run(true);
  // After construction GCMNetworkChannel should have called Register.
  EXPECT_FALSE(delegate()->register_callback.is_null());
  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  network_channel()->SendMessage("abra.cadabra");
  // SendMessage should have triggered RequestToken. No HTTP request should be
  // started yet.
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(url_fetchers_created_count(), 0);
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);

  // Return another access token. Message should be cleared by now and shouldn't
  // be sent.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token2");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);
  EXPECT_EQ(INVALIDATIONS_ENABLED, get_last_invalidator_state());
}

TEST_F(GCMNetworkChannelTest, FailedRegister) {
  // After construction GCMNetworkChannel should have called Register.
  EXPECT_FALSE(delegate()->register_callback.is_null());
  EXPECT_EQ(1, delegate()->register_call_count_);
  // Return transient error from Register call.
  delegate()->register_callback.Run("", gcm::GCMClient::NETWORK_ERROR);
  RunLoopUntilIdle();
  // GcmNetworkChannel should have scheduled Register retry.
  EXPECT_EQ(2, delegate()->register_call_count_);
  // Return persistent error from Register call.
  delegate()->register_callback.Run("", gcm::GCMClient::GCM_DISABLED);
  RunLoopUntilIdle();
  // GcmNetworkChannel should give up trying.
  EXPECT_EQ(2, delegate()->register_call_count_);

  network_channel()->SendMessage("abra.cadabra");
  // SendMessage shouldn't trigger RequestToken.
  EXPECT_TRUE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(0, url_fetchers_created_count());
}

TEST_F(GCMNetworkChannelTest, RegisterFinishesAfterSendMessage) {
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         "",
                                         net::HTTP_NO_CONTENT,
                                         net::URLRequestStatus::SUCCESS);

  // After construction GCMNetworkChannel should have called Register.
  EXPECT_FALSE(delegate()->register_callback.is_null());

  network_channel()->SendMessage("abra.cadabra");
  // SendMessage shouldn't trigger RequestToken.
  EXPECT_TRUE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(url_fetchers_created_count(), 0);

  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(url_fetchers_created_count(), 0);
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);
}

TEST_F(GCMNetworkChannelTest, RequestTokenFailure) {
  // After construction GCMNetworkChannel should have called Register.
  EXPECT_FALSE(delegate()->register_callback.is_null());
  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  network_channel()->SendMessage("abra.cadabra");
  // SendMessage should have triggered RequestToken. No HTTP request should be
  // started yet.
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(url_fetchers_created_count(), 0);
  // RequestToken returns failure.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::FromConnectionError(1), "");

  // Should be no HTTP requests.
  EXPECT_EQ(url_fetchers_created_count(), 0);
}

TEST_F(GCMNetworkChannelTest, AuthErrorFromServer) {
  // Setup fake response to return AUTH_ERROR.
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         "",
                                         net::HTTP_UNAUTHORIZED,
                                         net::URLRequestStatus::SUCCESS);

  // After construction GCMNetworkChannel should have called Register.
  EXPECT_FALSE(delegate()->register_callback.is_null());
  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  network_channel()->SendMessage("abra.cadabra");
  // SendMessage should have triggered RequestToken. No HTTP request should be
  // started yet.
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  EXPECT_EQ(url_fetchers_created_count(), 0);
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);
  EXPECT_EQ(delegate()->invalidated_token, "access.token");
}

// Following two tests are to check for memory leaks/crashes when Register and
// RequestToken don't complete by the teardown.
TEST_F(GCMNetworkChannelTest, RegisterNeverCompletes) {
  network_channel()->SendMessage("abra.cadabra");
  // Register should be called by now. Let's not complete and see what happens.
  EXPECT_FALSE(delegate()->register_callback.is_null());
}

TEST_F(GCMNetworkChannelTest, RequestTokenNeverCompletes) {
  network_channel()->SendMessage("abra.cadabra");
  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);
  // RequestToken should be called by now. Let's not complete and see what
  // happens.
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
}

TEST_F(GCMNetworkChannelTest, ChannelState) {
  EXPECT_FALSE(delegate()->message_callback.is_null());
  // POST will fail.
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         std::string(),
                                         net::HTTP_SERVICE_UNAVAILABLE,
                                         net::URLRequestStatus::SUCCESS);

  delegate()->connection_state_callback.Run(true);
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  network_channel()->SendMessage("abra.cadabra");
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);
  // Failing HTTP POST should cause TRANSIENT_INVALIDATION_ERROR.
  EXPECT_EQ(TRANSIENT_INVALIDATION_ERROR, get_last_invalidator_state());

  // Setup POST to succeed.
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         "",
                                         net::HTTP_NO_CONTENT,
                                         net::URLRequestStatus::SUCCESS);
  network_channel()->SendMessage("abra.cadabra");
  EXPECT_FALSE(delegate()->request_token_callback.is_null());
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 2);
  // Successful post should set invalidator state to enabled.
  EXPECT_EQ(INVALIDATIONS_ENABLED, get_last_invalidator_state());
  // Network changed event shouldn't affect invalidator state.
  network_channel()->OnNetworkChanged(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_EQ(INVALIDATIONS_ENABLED, get_last_invalidator_state());

  // GCM connection state should affect invalidator state.
  delegate()->connection_state_callback.Run(false);
  EXPECT_EQ(TRANSIENT_INVALIDATION_ERROR, get_last_invalidator_state());
  delegate()->connection_state_callback.Run(true);
  EXPECT_EQ(INVALIDATIONS_ENABLED, get_last_invalidator_state());
}

#if !defined(OS_ANDROID)
TEST_F(GCMNetworkChannelTest, BuildUrl) {
  GURL url = BuildUrl("registration.id");
  EXPECT_TRUE(url.SchemeIsHTTPOrHTTPS());
  EXPECT_FALSE(url.host().empty());
  EXPECT_FALSE(url.path().empty());
  std::vector<std::string> parts = base::SplitString(
      url.path(), "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::string buffer;
  EXPECT_TRUE(base::Base64UrlDecode(parts.back(),
                                    base::Base64UrlDecodePolicy::IGNORE_PADDING,
                                    &buffer));
}

TEST_F(GCMNetworkChannelTest, EchoToken) {
  url_fetcher_factory()->SetFakeResponse(GURL("http://test.url.com"),
                                         std::string(),
                                         net::HTTP_OK,
                                         net::URLRequestStatus::SUCCESS);
  // After construction GCMNetworkChannel should have called Register.
  // Return valid registration id.
  delegate()->register_callback.Run("registration.id", gcm::GCMClient::SUCCESS);

  network_channel()->SendMessage("abra.cadabra");
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 1);
  EXPECT_TRUE(get_last_echo_token().empty());

  // Trigger response.
  delegate()->message_callback.Run("abra.cadabra", "echo.token");
  // Send another message.
  network_channel()->SendMessage("abra.cadabra");
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 2);
  EXPECT_EQ("echo.token", get_last_echo_token());

  // Trigger response with empty echo token.
  delegate()->message_callback.Run("abra.cadabra", "");
  // Send another message.
  network_channel()->SendMessage("abra.cadabra");
  // Return valid access token. This should trigger HTTP request.
  delegate()->request_token_callback.Run(
      GoogleServiceAuthError::AuthErrorNone(), "access.token");
  RunLoopUntilIdle();
  EXPECT_EQ(url_fetchers_created_count(), 3);
  // Echo_token should be from second message.
  EXPECT_EQ("echo.token", get_last_echo_token());
}
#endif

}  // namespace syncer
