// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation/browser_presentation_connection_proxy.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/browser/media/router/test/test_helper.h"
#include "chrome/common/media_router/media_source.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "content/public/common/presentation_connection_message.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"

using ::testing::_;
using ::testing::Invoke;

namespace media_router {

using OnMessageCallback = BrowserPresentationConnectionProxy::OnMessageCallback;

namespace {

void ExpectMessageAndRunCallback(
    const content::PresentationConnectionMessage& expected_message,
    const content::PresentationConnectionMessage& message,
    OnMessageCallback& callback) {
  EXPECT_EQ(expected_message, message);
  std::move(callback).Run(true);
}

}  // namespace

constexpr char kMediaRouteId[] = "MockRouteId";

class BrowserPresentationConnectionProxyTest : public ::testing::Test {
 public:
  BrowserPresentationConnectionProxyTest() = default;

  void SetUp() override {
    mock_controller_connection_proxy_ =
        std::make_unique<MockPresentationConnectionProxy>();
    blink::mojom::PresentationConnectionPtr controller_connection_ptr;
    binding_.reset(new mojo::Binding<blink::mojom::PresentationConnection>(
        mock_controller_connection_proxy_.get(),
        mojo::MakeRequest(&controller_connection_ptr)));
    EXPECT_CALL(mock_router_, RegisterRouteMessageObserver(_));
    EXPECT_CALL(
        *mock_controller_connection_proxy_,
        DidChangeState(blink::mojom::PresentationConnectionState::CONNECTED));

    blink::mojom::PresentationConnectionPtr receiver_connection_ptr;

    base::RunLoop run_loop;
    browser_connection_proxy_ =
        std::make_unique<BrowserPresentationConnectionProxy>(
            &mock_router_, "MockRouteId",
            mojo::MakeRequest(&receiver_connection_ptr),
            std::move(controller_connection_ptr));
    run_loop.RunUntilIdle();
  }

  void TearDown() override {
    EXPECT_CALL(mock_router_, UnregisterRouteMessageObserver(_));
    browser_connection_proxy_.reset();
    binding_.reset();
    mock_controller_connection_proxy_.reset();
  }

  MockPresentationConnectionProxy* controller_connection_proxy() {
    return mock_controller_connection_proxy_.get();
  }

  BrowserPresentationConnectionProxy* browser_connection_proxy() {
    return browser_connection_proxy_.get();
  }

  MockMediaRouter* mock_router() { return &mock_router_; }

 private:
  std::unique_ptr<MockPresentationConnectionProxy>
      mock_controller_connection_proxy_;
  std::unique_ptr<mojo::Binding<blink::mojom::PresentationConnection>> binding_;
  std::unique_ptr<BrowserPresentationConnectionProxy> browser_connection_proxy_;
  MockMediaRouter mock_router_;
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(BrowserPresentationConnectionProxyTest, TestOnMessageTextMessage) {
  std::string message = "test message";
  content::PresentationConnectionMessage connection_message(message);

  base::MockCallback<base::OnceCallback<void(bool)>> mock_on_message_callback;
  EXPECT_CALL(*mock_router(),
              SendRouteMessageInternal(kMediaRouteId, message, _));

  browser_connection_proxy()->OnMessage(std::move(connection_message),
                                        mock_on_message_callback.Get());
}

TEST_F(BrowserPresentationConnectionProxyTest, TestOnMessageBinaryMessage) {
  std::vector<uint8_t> expected_data;
  expected_data.push_back(42);
  expected_data.push_back(36);

  content::PresentationConnectionMessage connection_message(expected_data);

  base::MockCallback<base::OnceCallback<void(bool)>> mock_on_message_callback;
  EXPECT_CALL(*mock_router(), SendRouteBinaryMessageInternal(_, _, _))
      .WillOnce(Invoke([&expected_data](const MediaRoute::Id& route_id,
                                        std::vector<uint8_t>* data,
                                        const OnMessageCallback& callback) {
        EXPECT_EQ(*data, expected_data);
      }));

  browser_connection_proxy()->OnMessage(std::move(connection_message),
                                        mock_on_message_callback.Get());
}

TEST_F(BrowserPresentationConnectionProxyTest, OnMessagesReceived) {
  content::PresentationConnectionMessage message_1;
  message_1.message = std::string("foo");
  content::PresentationConnectionMessage message_2;
  message_2.data = std::vector<uint8_t>({1, 2, 3});
  std::vector<content::PresentationConnectionMessage> messages = {message_1,
                                                                  message_2};

  EXPECT_CALL(*controller_connection_proxy(), OnMessageInternal(_, _))
      .WillOnce(Invoke(
          [&message_1](const content::PresentationConnectionMessage& message,
                       OnMessageCallback& callback) {
            ExpectMessageAndRunCallback(message_1, message, callback);
          }))
      .WillOnce(Invoke(
          [&message_2](const content::PresentationConnectionMessage& message,
                       OnMessageCallback& callback) {
            ExpectMessageAndRunCallback(message_2, message, callback);
          }));
  browser_connection_proxy()->OnMessagesReceived(messages);
  base::RunLoop().RunUntilIdle();
}

}  // namespace media_router
