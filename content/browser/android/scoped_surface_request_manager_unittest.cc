// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/scoped_surface_request_manager.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/run_loop.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"

namespace content {

class ScopedSurfaceRequestManagerUnitTest : public testing::Test {
 public:
  ScopedSurfaceRequestManagerUnitTest() {
    manager_ = ScopedSurfaceRequestManager::GetInstance();

    // The need to reset the callbacks because the
    // ScopedSurfaceRequestManager's lifetime outlive the tests.
    manager_->clear_requests_for_testing();

    last_received_request_ = 0;
    dummy_token_ = base::UnguessableToken::Deserialize(123, 456);

    surface_texture = gl::SurfaceTexture::Create(0);
    dummy_request_ =
        base::Bind(&ScopedSurfaceRequestManagerUnitTest::DummyCallback,
                   base::Unretained(this));
    specific_logging_request_ =
        base::Bind(&ScopedSurfaceRequestManagerUnitTest::LoggingCallback,
                   base::Unretained(this), kSpecificCallbackId);
  }

  // No-op callback.
  void DummyCallback(gl::ScopedJavaSurface surface) {}

  // Callback that updates |last_received_request_| to allow differentiation
  // between callback instances in tests.
  void LoggingCallback(int request_id, gl::ScopedJavaSurface surface) {
    last_received_request_ = request_id;
  }

  ScopedSurfaceRequestManager::ScopedSurfaceRequestCB dummy_request_;
  ScopedSurfaceRequestManager::ScopedSurfaceRequestCB specific_logging_request_;
  scoped_refptr<gl::SurfaceTexture> surface_texture;

  int last_received_request_;
  const int kSpecificCallbackId = 1357;
  base::UnguessableToken dummy_token_;

  ScopedSurfaceRequestManager* manager_;

  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSurfaceRequestManagerUnitTest);
};

// Makes sure we can successfully register a callback.
TEST_F(ScopedSurfaceRequestManagerUnitTest, RegisterRequest_ShouldSucceed) {
  EXPECT_EQ(0, manager_->request_count_for_testing());

  base::UnguessableToken token =
      manager_->RegisterScopedSurfaceRequest(dummy_request_);

  EXPECT_EQ(1, manager_->request_count_for_testing());
  EXPECT_FALSE(token.is_empty());
}

// Makes sure we can successfully register multiple callbacks, and that they
// return distinct request tokens.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       RegisterMultipleRequests_ShouldSucceed) {
  base::UnguessableToken token1 =
      manager_->RegisterScopedSurfaceRequest(dummy_request_);
  base::UnguessableToken token2 =
      manager_->RegisterScopedSurfaceRequest(dummy_request_);

  EXPECT_EQ(2, manager_->request_count_for_testing());
  EXPECT_NE(token1, token2);
}

// Makes sure GetInstance() is idempotent/that the class is a proper singleton.
TEST_F(ScopedSurfaceRequestManagerUnitTest, VerifySingleton_ShouldSucceed) {
  EXPECT_EQ(manager_, ScopedSurfaceRequestManager::GetInstance());
}

// Makes sure we can unregister a callback after registering it.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       GetRegisteredRequest_ShouldSucceed) {
  base::UnguessableToken token =
      manager_->RegisterScopedSurfaceRequest(dummy_request_);
  EXPECT_EQ(1, manager_->request_count_for_testing());

  manager_->UnregisterScopedSurfaceRequest(token);

  EXPECT_EQ(0, manager_->request_count_for_testing());
}

// Makes sure that unregistering a callback only affects the specified callback.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       GetRegisteredRequestFromMultipleRequests_ShouldSucceed) {
  base::UnguessableToken token =
      manager_->RegisterScopedSurfaceRequest(dummy_request_);
  manager_->RegisterScopedSurfaceRequest(dummy_request_);
  EXPECT_EQ(2, manager_->request_count_for_testing());

  manager_->UnregisterScopedSurfaceRequest(token);

  EXPECT_EQ(1, manager_->request_count_for_testing());
}

// Makes sure that unregistration is a noop permitted when there are no
// registered requests.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       UnregisteredRequest_ShouldReturnNullCallback) {
  manager_->UnregisterScopedSurfaceRequest(dummy_token_);

  EXPECT_EQ(0, manager_->request_count_for_testing());
}

// Makes sure that unregistering an invalid |request_token| doesn't affect
// other registered callbacks.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       GetUnregisteredRequestFromMultipleRequests_ShouldReturnNullCallback) {
  manager_->RegisterScopedSurfaceRequest(dummy_request_);

  manager_->UnregisterScopedSurfaceRequest(dummy_token_);

  EXPECT_EQ(1, manager_->request_count_for_testing());
}

// Makes sure that trying to fulfill a request for an invalid |request_token|
// does nothing, and does not affect other callbacks.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       FulfillUnregisteredRequest_ShouldDoNothing) {
  manager_->RegisterScopedSurfaceRequest(specific_logging_request_);

  manager_->FulfillScopedSurfaceRequest(
      dummy_token_, gl::ScopedJavaSurface(surface_texture.get()));

  EXPECT_EQ(1, manager_->request_count_for_testing());
  EXPECT_NE(kSpecificCallbackId, last_received_request_);
}

// Makes sure that trying to fulfill a request fulfills the right request, and
// does not affect other registered requests.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       FulfillRegisteredRequest_ShouldSucceed) {
  base::UnguessableToken specific_token =
      manager_->RegisterScopedSurfaceRequest(specific_logging_request_);

  const uint64_t kOtherCallbackId = 5678;
  manager_->RegisterScopedSurfaceRequest(
      base::Bind(&ScopedSurfaceRequestManagerUnitTest::LoggingCallback,
                 base::Unretained(this), kOtherCallbackId));

  manager_->FulfillScopedSurfaceRequest(
      specific_token, gl::ScopedJavaSurface(surface_texture.get()));

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, manager_->request_count_for_testing());
  EXPECT_EQ(kSpecificCallbackId, last_received_request_);
}

// Makes sure that the ScopedSurfaceRequestConduit implementation properly
// fulfills requests.
TEST_F(ScopedSurfaceRequestManagerUnitTest,
       ForwardSurfaceTexture_ShouldFulfillRequest) {
  base::UnguessableToken token =
      manager_->RegisterScopedSurfaceRequest(specific_logging_request_);

  manager_->ForwardSurfaceTextureForSurfaceRequest(token,
                                                   surface_texture.get());

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, manager_->request_count_for_testing());
  EXPECT_EQ(kSpecificCallbackId, last_received_request_);
}

}  // Content
