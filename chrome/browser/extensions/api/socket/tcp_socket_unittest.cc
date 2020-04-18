// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "extensions/browser/api/socket/tcp_socket.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;

namespace extensions {

class MockTCPSocket : public net::TCPClientSocket {
 public:
  explicit MockTCPSocket(const net::AddressList& address_list)
      : net::TCPClientSocket(address_list, NULL, NULL, net::NetLogSource()) {}

  int Read(net::IOBuffer* buffer,
           int bytes,
           net::CompletionOnceCallback callback) override {
    return Read(buffer, bytes,
                base::AdaptCallbackForRepeating(std::move(callback)));
  }

  int Write(net::IOBuffer* buffer,
            int bytes,
            net::CompletionOnceCallback callback,
            const net::NetworkTrafficAnnotationTag& tag) override {
    return Write(buffer, bytes,
                 base::AdaptCallbackForRepeating(std::move(callback)), tag);
  }

  MOCK_METHOD3(Read, int(net::IOBuffer* buf, int buf_len,
                         const net::CompletionCallback& callback));
  MOCK_METHOD4(Write,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback,
                   const net::NetworkTrafficAnnotationTag&));
  MOCK_METHOD2(SetKeepAlive, bool(bool enable, int delay));
  MOCK_METHOD1(SetNoDelay, bool(bool no_delay));
  bool IsConnected() const override {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTCPSocket);
};

class MockTCPServerSocket : public net::TCPServerSocket {
 public:
  MockTCPServerSocket() : net::TCPServerSocket(NULL, net::NetLogSource()) {}
  MOCK_METHOD2(Listen, int(const net::IPEndPoint& address, int backlog));
  MOCK_METHOD2(Accept,
               int(std::unique_ptr<net::StreamSocket>* socket,
                   const net::CompletionCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTCPServerSocket);
};

class CompleteHandler {
 public:
  CompleteHandler() {}
  MOCK_METHOD1(OnComplete, void(int result_code));
  MOCK_METHOD3(OnReadComplete,
               void(int result_code,
                    scoped_refptr<net::IOBuffer> io_buffer,
                    bool socket_destroying));

  // MOCK_METHOD cannot mock a scoped_ptr argument.
  MOCK_METHOD2(OnAcceptMock, void(int, net::TCPClientSocket*));
  void OnAccept(int count, std::unique_ptr<net::TCPClientSocket> socket) {
    OnAcceptMock(count, socket.get());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CompleteHandler);
};

const char FAKE_ID[] = "abcdefghijklmnopqrst";

TEST(SocketTest, TestTCPSocketRead) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));
  CompleteHandler handler;

  EXPECT_CALL(*tcp_client_socket, Read(_, _, _))
      .Times(1);
  EXPECT_CALL(handler, OnReadComplete(_, _, _)).Times(1);

  std::unique_ptr<TCPSocket> socket(TCPSocket::CreateSocketForTesting(
      std::move(tcp_client_socket), FAKE_ID, true));

  const int count = 512;
  socket->Read(count, base::Bind(&CompleteHandler::OnReadComplete,
        base::Unretained(&handler)));
}

TEST(SocketTest, TestTCPSocketWrite) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));
  CompleteHandler handler;

  net::CompletionCallback callback;
  EXPECT_CALL(*tcp_client_socket, Write(_, _, _, _))
      .Times(2)
      .WillRepeatedly(testing::DoAll(SaveArg<2>(&callback), Return(128)));
  EXPECT_CALL(handler, OnComplete(_))
      .Times(1);

  std::unique_ptr<TCPSocket> socket(TCPSocket::CreateSocketForTesting(
      std::move(tcp_client_socket), FAKE_ID, true));

  scoped_refptr<net::IOBufferWithSize> io_buffer(
      new net::IOBufferWithSize(256));
  socket->Write(io_buffer.get(), io_buffer->size(),
      base::Bind(&CompleteHandler::OnComplete, base::Unretained(&handler)));
}

TEST(SocketTest, TestTCPSocketBlockedWrite) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));
  CompleteHandler handler;

  net::CompletionCallback callback;
  EXPECT_CALL(*tcp_client_socket, Write(_, _, _, _))
      .Times(2)
      .WillRepeatedly(
          testing::DoAll(SaveArg<2>(&callback), Return(net::ERR_IO_PENDING)));

  std::unique_ptr<TCPSocket> socket(TCPSocket::CreateSocketForTesting(
      std::move(tcp_client_socket), FAKE_ID, true));

  scoped_refptr<net::IOBufferWithSize> io_buffer(new net::IOBufferWithSize(42));
  socket->Write(io_buffer.get(), io_buffer->size(),
      base::Bind(&CompleteHandler::OnComplete, base::Unretained(&handler)));

  // Good. Original call came back unable to complete. Now pretend the socket
  // finished, and confirm that we passed the error back.
  EXPECT_CALL(handler, OnComplete(42))
      .Times(1);
  callback.Run(40);
  callback.Run(2);
}

TEST(SocketTest, TestTCPSocketBlockedWriteReentry) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));
  CompleteHandler handlers[5];

  net::CompletionCallback callback;
  EXPECT_CALL(*tcp_client_socket, Write(_, _, _, _))
      .Times(5)
      .WillRepeatedly(
          testing::DoAll(SaveArg<2>(&callback), Return(net::ERR_IO_PENDING)));

  std::unique_ptr<TCPSocket> socket(TCPSocket::CreateSocketForTesting(
      std::move(tcp_client_socket), FAKE_ID, true));

  scoped_refptr<net::IOBufferWithSize> io_buffers[5];
  int i;
  for (i = 0; i < 5; i++) {
    io_buffers[i] = new net::IOBufferWithSize(128 + i * 50);
    scoped_refptr<net::IOBufferWithSize> io_buffer1(
        new net::IOBufferWithSize(42));
    socket->Write(io_buffers[i].get(), io_buffers[i]->size(),
        base::Bind(&CompleteHandler::OnComplete,
            base::Unretained(&handlers[i])));

    EXPECT_CALL(handlers[i], OnComplete(io_buffers[i]->size()))
        .Times(1);
  }

  for (i = 0; i < 5; i++) {
    callback.Run(128 + i * 50);
  }
}

TEST(SocketTest, TestTCPSocketSetNoDelay) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));

  bool no_delay = false;
  {
    testing::InSequence dummy;
    EXPECT_CALL(*tcp_client_socket, SetNoDelay(_))
        .WillOnce(testing::DoAll(SaveArg<0>(&no_delay), Return(true)));
    EXPECT_CALL(*tcp_client_socket, SetNoDelay(_))
        .WillOnce(testing::DoAll(SaveArg<0>(&no_delay), Return(false)));
  }

  std::unique_ptr<TCPSocket> socket(
      TCPSocket::CreateSocketForTesting(std::move(tcp_client_socket), FAKE_ID));

  EXPECT_FALSE(no_delay);
  int result = socket->SetNoDelay(true);
  EXPECT_TRUE(result);
  EXPECT_TRUE(no_delay);

  result = socket->SetNoDelay(false);
  EXPECT_FALSE(result);
  EXPECT_FALSE(no_delay);
}

TEST(SocketTest, TestTCPSocketSetKeepAlive) {
  net::AddressList address_list;
  std::unique_ptr<MockTCPSocket> tcp_client_socket(
      new MockTCPSocket(address_list));

  bool enable = false;
  int delay = 0;
  {
    testing::InSequence dummy;
    EXPECT_CALL(*tcp_client_socket, SetKeepAlive(_, _))
        .WillOnce(testing::DoAll(SaveArg<0>(&enable), SaveArg<1>(&delay),
                                 Return(true)));
    EXPECT_CALL(*tcp_client_socket, SetKeepAlive(_, _))
        .WillOnce(testing::DoAll(SaveArg<0>(&enable), SaveArg<1>(&delay),
                                 Return(false)));
  }

  std::unique_ptr<TCPSocket> socket(
      TCPSocket::CreateSocketForTesting(std::move(tcp_client_socket), FAKE_ID));

  EXPECT_FALSE(enable);
  int result = socket->SetKeepAlive(true, 4500);
  EXPECT_TRUE(result);
  EXPECT_TRUE(enable);
  EXPECT_EQ(4500, delay);

  result = socket->SetKeepAlive(false, 0);
  EXPECT_FALSE(result);
  EXPECT_FALSE(enable);
  EXPECT_EQ(0, delay);
}

TEST(SocketTest, TestTCPServerSocketListenAccept) {
  std::unique_ptr<MockTCPServerSocket> tcp_server_socket(
      new MockTCPServerSocket());
  CompleteHandler handler;

  EXPECT_CALL(*tcp_server_socket, Accept(_, _)).Times(1);
  EXPECT_CALL(*tcp_server_socket, Listen(_, _)).Times(1);

  std::unique_ptr<TCPSocket> socket(TCPSocket::CreateServerSocketForTesting(
      std::move(tcp_server_socket), FAKE_ID));

  EXPECT_CALL(handler, OnAcceptMock(_, _));

  std::string err_msg;
  EXPECT_EQ(net::OK, socket->Listen("127.0.0.1", 9999, 10, &err_msg));
  socket->Accept(base::Bind(&CompleteHandler::OnAccept,
        base::Unretained(&handler)));
}

}  // namespace extensions
