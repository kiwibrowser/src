// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/gcm_invalidation_bridge.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/gcm_driver/fake_gcm_driver.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "google_apis/gaia/fake_identity_provider.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/ip_endpoint.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace invalidation {
namespace {

// Implementation of GCMDriver::Register that always succeeds with the same
// registrationId.
class CustomFakeGCMDriver : public gcm::FakeGCMDriver {
 public:
  CustomFakeGCMDriver() {}
  ~CustomFakeGCMDriver() override {}

 protected:
  // FakeGCMDriver override:
  void RegisterImpl(const std::string& app_id,
                    const std::vector<std::string>& sender_ids) override {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&CustomFakeGCMDriver::RegisterFinished,
                   base::Unretained(this), app_id,
                   std::string("registration.id"), gcm::GCMClient::SUCCESS));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CustomFakeGCMDriver);
};

class GCMInvalidationBridgeTest : public ::testing::Test {
 protected:
  GCMInvalidationBridgeTest()
      : connection_online_(false) {}

  ~GCMInvalidationBridgeTest() override {}

  void SetUp() override {
    token_service_.reset(new FakeProfileOAuth2TokenService());
    token_service_->set_auto_post_fetch_response_on_message_loop(true);
    token_service_->UpdateCredentials("", "fake_refresh_token");
    gcm_driver_.reset(new CustomFakeGCMDriver());

    identity_provider_.reset(new FakeIdentityProvider(token_service_.get()));
    bridge_.reset(new GCMInvalidationBridge(gcm_driver_.get(),
                                            identity_provider_.get()));

    delegate_ = bridge_->CreateDelegate();
    delegate_->Initialize(
        base::Bind(&GCMInvalidationBridgeTest::ConnectionStateChanged,
                   base::Unretained(this)),
        base::DoNothing() /* store_reset_callback */);
    RunLoop();
  }

  void RunLoop() {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

 public:
  void RegisterFinished(const std::string& registration_id,
                        gcm::GCMClient::Result result) {
    registration_id_ = registration_id;
  }

  void RequestTokenFinished(const GoogleServiceAuthError& error,
                            const std::string& token) {
    issued_tokens_.push_back(token);
    request_token_errors_.push_back(error);
  }

  void ConnectionStateChanged(bool online) {
    connection_online_ = online;
  }

  base::MessageLoop message_loop_;
  std::unique_ptr<FakeProfileOAuth2TokenService> token_service_;
  std::unique_ptr<gcm::GCMDriver> gcm_driver_;
  std::unique_ptr<FakeIdentityProvider> identity_provider_;

  std::vector<std::string> issued_tokens_;
  std::vector<GoogleServiceAuthError> request_token_errors_;
  std::string registration_id_;
  bool connection_online_;

  std::unique_ptr<GCMInvalidationBridge> bridge_;
  std::unique_ptr<syncer::GCMNetworkChannelDelegate> delegate_;
};

TEST_F(GCMInvalidationBridgeTest, RequestToken) {
  // Make sure that call to RequestToken reaches OAuth2TokenService and gets
  // back to callback.
  delegate_->RequestToken(
      base::Bind(&GCMInvalidationBridgeTest::RequestTokenFinished,
                 base::Unretained(this)));
  RunLoop();
  EXPECT_EQ(1U, issued_tokens_.size());
  EXPECT_NE("", issued_tokens_[0]);
  EXPECT_EQ(GoogleServiceAuthError::AuthErrorNone(), request_token_errors_[0]);
}

TEST_F(GCMInvalidationBridgeTest, RequestTokenTwoConcurrentRequests) {
  // First call should finish with REQUEST_CANCELLED error.
  delegate_->RequestToken(
      base::Bind(&GCMInvalidationBridgeTest::RequestTokenFinished,
                 base::Unretained(this)));
  // Second request should succeed.
  delegate_->RequestToken(
      base::Bind(&GCMInvalidationBridgeTest::RequestTokenFinished,
                 base::Unretained(this)));
  RunLoop();

  EXPECT_EQ(2U, issued_tokens_.size());

  EXPECT_EQ("", issued_tokens_[0]);
  EXPECT_EQ(GoogleServiceAuthError::REQUEST_CANCELED,
            request_token_errors_[0].state());

  EXPECT_NE("", issued_tokens_[1]);
  EXPECT_EQ(GoogleServiceAuthError::AuthErrorNone(), request_token_errors_[1]);
}

TEST_F(GCMInvalidationBridgeTest, Register) {
  EXPECT_TRUE(registration_id_.empty());
  delegate_->Register(base::Bind(&GCMInvalidationBridgeTest::RegisterFinished,
                                 base::Unretained(this)));
  RunLoop();

  EXPECT_FALSE(registration_id_.empty());
}

TEST_F(GCMInvalidationBridgeTest, ConnectionState) {
  EXPECT_FALSE(connection_online_);
  bridge_->OnConnected(net::IPEndPoint());
  RunLoop();
  EXPECT_TRUE(connection_online_);
  bridge_->OnDisconnected();
  RunLoop();
  EXPECT_FALSE(connection_online_);
}

}  // namespace
}  // namespace invalidation
