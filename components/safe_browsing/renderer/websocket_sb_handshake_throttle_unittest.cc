// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/renderer/websocket_sb_handshake_throttle.h"

#include <utility>

#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/safe_browsing/common/safe_browsing.mojom.h"
#include "content/public/common/resource_type.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/http/http_request_headers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

namespace safe_browsing {

namespace {

constexpr char kTestUrl[] = "wss://test/";

class FakeSafeBrowsing : public mojom::SafeBrowsing {
 public:
  FakeSafeBrowsing()
      : render_frame_id_(),
        load_flags_(-1),
        resource_type_(),
        has_user_gesture_(false),
        originated_from_service_worker_(false) {}

  void CreateCheckerAndCheck(int32_t render_frame_id,
                             mojom::SafeBrowsingUrlCheckerRequest request,
                             const GURL& url,
                             const std::string& method,
                             const net::HttpRequestHeaders& headers,
                             int32_t load_flags,
                             content::ResourceType resource_type,
                             bool has_user_gesture,
                             bool originated_from_service_worker,
                             CreateCheckerAndCheckCallback callback) override {
    render_frame_id_ = render_frame_id;
    request_ = std::move(request);
    url_ = url;
    method_ = method;
    headers_ = headers;
    load_flags_ = load_flags;
    resource_type_ = resource_type;
    has_user_gesture_ = has_user_gesture;
    originated_from_service_worker_ = originated_from_service_worker;
    callback_ = std::move(callback);
    run_loop_.Quit();
  }

  void Clone(mojom::SafeBrowsingRequest request) override { NOTREACHED(); }

  void RunUntilCalled() { run_loop_.Run(); }

  int32_t render_frame_id_;
  mojom::SafeBrowsingUrlCheckerRequest request_;
  GURL url_;
  std::string method_;
  net::HttpRequestHeaders headers_;
  int32_t load_flags_;
  content::ResourceType resource_type_;
  bool has_user_gesture_;
  bool originated_from_service_worker_;
  CreateCheckerAndCheckCallback callback_;
  base::RunLoop run_loop_;
};

class FakeWebCallbacks
    : public blink::WebCallbacks<void, const blink::WebString&> {
 public:
  enum Result { RESULT_NOT_CALLED, RESULT_SUCCESS, RESULT_ERROR };

  FakeWebCallbacks() : result_(RESULT_NOT_CALLED) {}

  void OnSuccess() override {
    result_ = RESULT_SUCCESS;
    run_loop_.Quit();
  }

  void OnError(const blink::WebString& message) override {
    result_ = RESULT_ERROR;
    message_ = message;
    run_loop_.Quit();
  }

  void RunUntilCalled() { run_loop_.Run(); }

  void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  Result result_;
  blink::WebString message_;
  base::RunLoop run_loop_;
};

class WebSocketSBHandshakeThrottleTest : public ::testing::Test {
 protected:
  WebSocketSBHandshakeThrottleTest() : mojo_binding_(&safe_browsing_) {
    mojo_binding_.Bind(mojo::MakeRequest(&safe_browsing_ptr_));
    throttle_ = std::make_unique<WebSocketSBHandshakeThrottle>(
        safe_browsing_ptr_.get(), MSG_ROUTING_NONE);
  }

  base::test::ScopedTaskEnvironment message_loop_;
  FakeSafeBrowsing safe_browsing_;
  mojo::Binding<mojom::SafeBrowsing> mojo_binding_;
  mojom::SafeBrowsingPtr safe_browsing_ptr_;
  std::unique_ptr<WebSocketSBHandshakeThrottle> throttle_;
  FakeWebCallbacks fake_callbacks_;
};

TEST_F(WebSocketSBHandshakeThrottleTest, Construction) {}

TEST_F(WebSocketSBHandshakeThrottleTest, CheckArguments) {
  throttle_->ThrottleHandshake(GURL(kTestUrl), &fake_callbacks_);
  safe_browsing_.RunUntilCalled();
  EXPECT_EQ(MSG_ROUTING_NONE, safe_browsing_.render_frame_id_);
  EXPECT_EQ(GURL(kTestUrl), safe_browsing_.url_);
  EXPECT_EQ("GET", safe_browsing_.method_);
  EXPECT_TRUE(safe_browsing_.headers_.GetHeaderVector().empty());
  EXPECT_EQ(0, safe_browsing_.load_flags_);
  EXPECT_EQ(content::RESOURCE_TYPE_SUB_RESOURCE, safe_browsing_.resource_type_);
  EXPECT_FALSE(safe_browsing_.has_user_gesture_);
  EXPECT_FALSE(safe_browsing_.originated_from_service_worker_);
  EXPECT_TRUE(safe_browsing_.callback_);
}

TEST_F(WebSocketSBHandshakeThrottleTest, Safe) {
  throttle_->ThrottleHandshake(GURL(kTestUrl), &fake_callbacks_);
  safe_browsing_.RunUntilCalled();
  std::move(safe_browsing_.callback_).Run(nullptr, true, false);
  fake_callbacks_.RunUntilCalled();
  EXPECT_EQ(FakeWebCallbacks::RESULT_SUCCESS, fake_callbacks_.result_);
}

TEST_F(WebSocketSBHandshakeThrottleTest, Unsafe) {
  throttle_->ThrottleHandshake(GURL(kTestUrl), &fake_callbacks_);
  safe_browsing_.RunUntilCalled();
  std::move(safe_browsing_.callback_).Run(nullptr, false, false);
  fake_callbacks_.RunUntilCalled();
  EXPECT_EQ(FakeWebCallbacks::RESULT_ERROR, fake_callbacks_.result_);
  EXPECT_EQ(
      blink::WebString(
          "WebSocket connection to wss://test/ failed safe browsing check"),
      fake_callbacks_.message_);
}

TEST_F(WebSocketSBHandshakeThrottleTest, SlowCheckNotifier) {
  throttle_->ThrottleHandshake(GURL(kTestUrl), &fake_callbacks_);
  safe_browsing_.RunUntilCalled();

  mojom::UrlCheckNotifierPtr slow_check_notifier;
  std::move(safe_browsing_.callback_)
      .Run(mojo::MakeRequest(&slow_check_notifier), false, false);
  fake_callbacks_.RunUntilIdle();
  EXPECT_EQ(FakeWebCallbacks::RESULT_NOT_CALLED, fake_callbacks_.result_);

  slow_check_notifier->OnCompleteCheck(true, false);
  fake_callbacks_.RunUntilCalled();
  EXPECT_EQ(FakeWebCallbacks::RESULT_SUCCESS, fake_callbacks_.result_);
}

TEST_F(WebSocketSBHandshakeThrottleTest, MojoServiceNotThere) {
  mojo_binding_.Close();
  throttle_->ThrottleHandshake(GURL(kTestUrl), &fake_callbacks_);
  fake_callbacks_.RunUntilCalled();
  EXPECT_EQ(FakeWebCallbacks::RESULT_SUCCESS, fake_callbacks_.result_);
}

}  // namespace

}  // namespace safe_browsing
