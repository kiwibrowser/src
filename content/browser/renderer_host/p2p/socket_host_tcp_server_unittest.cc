// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/p2p/socket_host_tcp_server.h"

#include <stdint.h>

#include <list>

#include "content/browser/renderer_host/p2p/socket_host_tcp.h"
#include "content/browser/renderer_host/p2p/socket_host_test_utils.h"
#include "net/base/completion_callback.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DeleteArg;
using ::testing::DoAll;
using ::testing::Return;

namespace {

class FakeServerSocket : public net::ServerSocket {
 public:
  FakeServerSocket() : listening_(false), accept_socket_(nullptr) {}

  ~FakeServerSocket() override {}

  bool listening() { return listening_; }

  void AddIncoming(net::StreamSocket* socket) {
    if (!accept_callback_.is_null()) {
      DCHECK(incoming_sockets_.empty());
      accept_socket_->reset(socket);
      accept_socket_ = nullptr;

      // This copy is necessary because this implementation of ServerSocket
      // bases logic on the null-ness of |accept_callback_| in the bound
      // callback.
      net::CompletionCallback cb = accept_callback_;
      accept_callback_.Reset();
      std::move(cb).Run(net::OK);
    } else {
      incoming_sockets_.push_back(socket);
    }
  }

  int Listen(const net::IPEndPoint& address, int backlog) override {
    local_address_ = address;
    listening_ = true;
    return net::OK;
  }

  int GetLocalAddress(net::IPEndPoint* address) const override {
    *address = local_address_;
    return net::OK;
  }

  int Accept(std::unique_ptr<net::StreamSocket>* socket,
             const net::CompletionCallback& callback) override {
    DCHECK(socket);
    if (!incoming_sockets_.empty()) {
      socket->reset(incoming_sockets_.front());
      incoming_sockets_.pop_front();
      return net::OK;
    } else {
      accept_socket_ = socket;
      accept_callback_ = callback;
      return net::ERR_IO_PENDING;
    }
  }

 private:
  bool listening_;

  net::IPEndPoint local_address_;

  std::unique_ptr<net::StreamSocket>* accept_socket_;
  net::CompletionCallback accept_callback_;

  std::list<net::StreamSocket*> incoming_sockets_;
};

}  // namespace

namespace content {

class P2PSocketHostTcpServerTest : public testing::Test {
 protected:
  void SetUp() override {
    socket_ = new FakeServerSocket();
    socket_host_.reset(
        new P2PSocketHostTcpServer(&sender_, 0, P2P_SOCKET_TCP_CLIENT));
    socket_host_->socket_.reset(socket_);

    EXPECT_CALL(
        sender_,
        Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
        .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

    P2PHostAndIPEndPoint dest;
    dest.ip_address = ParseAddress(kTestIpAddress1, kTestPort1);

    socket_host_->Init(ParseAddress(kTestLocalIpAddress, 0), 0, 0, dest);
    EXPECT_TRUE(socket_->listening());
  }

  // Needed by the chilt classes because only this class is a friend
  // of P2PSocketHostTcp.
  net::StreamSocket* GetSocketFormTcpSocketHost(P2PSocketHostTcp* host) {
    return host->socket_.get();
  }

  MockIPCSender sender_;
  FakeServerSocket* socket_;  // Owned by |socket_host_|.
  std::unique_ptr<P2PSocketHostTcpServer> socket_host_;
};

// Accept incoming connection.
TEST_F(P2PSocketHostTcpServerTest, Accept) {
  FakeSocket* incoming = new FakeSocket(nullptr);
  incoming->SetLocalAddress(ParseAddress(kTestLocalIpAddress, kTestPort1));
  net::IPEndPoint addr = ParseAddress(kTestIpAddress1, kTestPort1);
  incoming->SetPeerAddress(addr);

  EXPECT_CALL(sender_, Send(MatchIncomingSocketMessage(addr)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->AddIncoming(incoming);

  const int kAcceptedSocketId = 1;

  std::unique_ptr<P2PSocketHost> new_host(
      socket_host_->AcceptIncomingTcpConnection(addr, kAcceptedSocketId));
  ASSERT_TRUE(new_host.get() != nullptr);
  EXPECT_EQ(incoming, GetSocketFormTcpSocketHost(
      reinterpret_cast<P2PSocketHostTcp*>(new_host.get())));
}

// Accept 2 simultaneous connections.
TEST_F(P2PSocketHostTcpServerTest, Accept2) {
  FakeSocket* incoming1 = new FakeSocket(nullptr);
  incoming1->SetLocalAddress(ParseAddress(kTestLocalIpAddress, kTestPort1));
  net::IPEndPoint addr1 = ParseAddress(kTestIpAddress1, kTestPort1);
  incoming1->SetPeerAddress(addr1);
  FakeSocket* incoming2 = new FakeSocket(nullptr);
  incoming2->SetLocalAddress(ParseAddress(kTestLocalIpAddress, kTestPort1));
  net::IPEndPoint addr2 = ParseAddress(kTestIpAddress2, kTestPort2);
  incoming2->SetPeerAddress(addr2);

  EXPECT_CALL(sender_, Send(MatchIncomingSocketMessage(addr1)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchIncomingSocketMessage(addr2)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->AddIncoming(incoming1);
  socket_->AddIncoming(incoming2);

  const int kAcceptedSocketId1 = 1;
  const int kAcceptedSocketId2 = 2;

  std::unique_ptr<P2PSocketHost> new_host1(
      socket_host_->AcceptIncomingTcpConnection(addr1, kAcceptedSocketId1));
  ASSERT_TRUE(new_host1.get() != nullptr);
  EXPECT_EQ(incoming1, GetSocketFormTcpSocketHost(
      reinterpret_cast<P2PSocketHostTcp*>(new_host1.get())));
  std::unique_ptr<P2PSocketHost> new_host2(
      socket_host_->AcceptIncomingTcpConnection(addr2, kAcceptedSocketId2));
  ASSERT_TRUE(new_host2.get() != nullptr);
  EXPECT_EQ(incoming2, GetSocketFormTcpSocketHost(
      reinterpret_cast<P2PSocketHostTcp*>(new_host2.get())));
}

}  // namespace content
