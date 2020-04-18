// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <vector>

#include "content/browser/websockets/websocket_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace {

// This number is unlikely to occur by chance.
static const int kMagicRenderProcessId = 506116062;

class TestWebSocketImpl : public network::WebSocket {
 public:
  TestWebSocketImpl(
      std::unique_ptr<Delegate> delegate,
      network::mojom::WebSocketRequest request,
      network::WebSocketThrottler::PendingConnection pending_connection_tracker,

      int process_id,
      int frame_id,
      url::Origin origin,
      base::TimeDelta delay)
      : network::WebSocket(std::move(delegate),
                           std::move(request),
                           std::move(pending_connection_tracker),
                           process_id,
                           frame_id,
                           std::move(origin),
                           delay) {}

  base::TimeDelta delay() const { return delay_; }

  void SimulateConnectionError() {
    OnConnectionError();
  }
};

class TestWebSocketManager : public WebSocketManager {
 public:
  TestWebSocketManager()
      : WebSocketManager(kMagicRenderProcessId, nullptr) {}

  const std::vector<TestWebSocketImpl*>& sockets() const {
    return sockets_;
  }

  int64_t num_pending_connections() const {
    return throttler_.num_pending_connections();
  }
  int64_t num_current_succeeded_connections() const {
    return throttler_.num_current_succeeded_connections();
  }
  int64_t num_previous_succeeded_connections() const {
    return throttler_.num_previous_succeeded_connections();
  }
  int64_t num_current_failed_connections() const {
    return throttler_.num_current_failed_connections();
  }
  int64_t num_previous_failed_connections() const {
    return throttler_.num_previous_failed_connections();
  }

  void DoCreateWebSocket(network::mojom::WebSocketRequest request) {
    WebSocketManager::DoCreateWebSocket(MSG_ROUTING_NONE, url::Origin(),
                                        std::move(request));
  }

 private:
  std::unique_ptr<network::WebSocket> DoCreateWebSocketInternal(
      std::unique_ptr<network::WebSocket::Delegate> delegate,
      network::mojom::WebSocketRequest request,
      network::WebSocketThrottler::PendingConnection pending_connection_tracker,
      int process_id,
      int frame_id,
      url::Origin origin,
      base::TimeDelta delay) override {
    auto impl = std::make_unique<TestWebSocketImpl>(
        std::move(delegate), std::move(request),
        std::move(pending_connection_tracker), process_id, frame_id,
        std::move(origin), delay);
    // We keep a vector of sockets here to track their creation order.
    sockets_.push_back(impl.get());
    return impl;
  }

  void OnLostConnectionToClient(network::WebSocket* impl) override {
    auto it = std::find(sockets_.begin(), sockets_.end(),
                        static_cast<TestWebSocketImpl*>(impl));
    ASSERT_TRUE(it != sockets_.end());
    sockets_.erase(it);

    WebSocketManager::OnLostConnectionToClient(impl);
  }

  std::vector<TestWebSocketImpl*> sockets_;
};

class WebSocketManagerTest : public ::testing::Test {
 public:
  WebSocketManagerTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {
    websocket_manager_.reset(new TestWebSocketManager());
  }

  void AddMultipleChannels(int number_of_channels) {
    for (int i = 0; i < number_of_channels; ++i) {
      network::mojom::WebSocketPtr websocket;
      websocket_manager_->DoCreateWebSocket(mojo::MakeRequest(&websocket));
    }
  }

  void AddAndCancelMultipleChannels(int number_of_channels) {
    for (int i = 0; i < number_of_channels; ++i) {
      network::mojom::WebSocketPtr websocket;
      websocket_manager_->DoCreateWebSocket(mojo::MakeRequest(&websocket));
      websocket_manager_->sockets().back()->SimulateConnectionError();
    }
  }

  TestWebSocketManager* websocket_manager() { return websocket_manager_.get(); }

 private:
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestWebSocketManager> websocket_manager_;
};

TEST_F(WebSocketManagerTest, Construct) {
  // Do nothing.
}

TEST_F(WebSocketManagerTest, CreateWebSocket) {
  network::mojom::WebSocketPtr websocket;

  websocket_manager()->DoCreateWebSocket(mojo::MakeRequest(&websocket));

  EXPECT_EQ(1U, websocket_manager()->sockets().size());
}

TEST_F(WebSocketManagerTest, SendFrameButNotConnectedYet) {
  network::mojom::WebSocketPtr websocket;

  websocket_manager()->DoCreateWebSocket(mojo::MakeRequest(&websocket));

  // This should not crash.
  std::vector<uint8_t> data;
  websocket->SendFrame(true, network::mojom::WebSocketMessageType::TEXT, data);
}

// The 256th connection is rejected by per-renderer WebSocket throttling.
// This is not counted as a failure.
TEST_F(WebSocketManagerTest, Rejects256thPendingConnection) {
  AddMultipleChannels(256);

  EXPECT_EQ(255, websocket_manager()->num_pending_connections());
  EXPECT_EQ(0, websocket_manager()->num_current_succeeded_connections());
  EXPECT_EQ(0, websocket_manager()->num_previous_succeeded_connections());
  EXPECT_EQ(0, websocket_manager()->num_current_failed_connections());
  EXPECT_EQ(0, websocket_manager()->num_previous_failed_connections());

  ASSERT_EQ(255U, websocket_manager()->sockets().size());
}

}  // namespace
}  // namespace content
