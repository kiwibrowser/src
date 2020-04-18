// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SOCKET_MOCK_TCP_CLIENT_SOCKET_H_
#define CHROME_BROWSER_EXTENSIONS_API_SOCKET_MOCK_TCP_CLIENT_SOCKET_H_

#include "base/callback_helpers.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace extensions {
class MockTCPClientSocket : public net::TCPClientSocket {
 public:
  MockTCPClientSocket();
  ~MockTCPClientSocket() override;

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

  int Connect(net::CompletionOnceCallback callback) override {
    return Connect(base::AdaptCallbackForRepeating(std::move(callback)));
  }

  MOCK_METHOD3(Read, int(net::IOBuffer*, int, const net::CompletionCallback&));
  MOCK_METHOD4(Write,
               int(net::IOBuffer*,
                   int,
                   const net::CompletionCallback&,
                   const net::NetworkTrafficAnnotationTag&));
  MOCK_METHOD1(SetReceiveBufferSize, int(int32_t));
  MOCK_METHOD1(SetSendBufferSize, int(int32_t));
  MOCK_METHOD1(Connect, int(const net::CompletionCallback&));
  MOCK_METHOD0(Disconnect, void());
  MOCK_CONST_METHOD0(IsConnected, bool());
  MOCK_CONST_METHOD0(IsConnectedAndIdle, bool());
  MOCK_CONST_METHOD1(GetPeerAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD1(GetLocalAddress, int(net::IPEndPoint*));
  MOCK_CONST_METHOD0(NetLog, const net::NetLogWithSource&());
  MOCK_CONST_METHOD0(WasEverUsed, bool());
  MOCK_CONST_METHOD0(UsingTCPFastOpen, bool());
  MOCK_CONST_METHOD0(NumBytesRead, int64_t());
  MOCK_CONST_METHOD0(GetConnectTimeMicros, base::TimeDelta());
  MOCK_CONST_METHOD0(WasAlpnNegotiated, bool());
  MOCK_CONST_METHOD0(GetNegotiatedProtocol, net::NextProto());
  MOCK_METHOD1(GetSSLInfo, bool(net::SSLInfo*));
  MOCK_CONST_METHOD1(GetConnectionAttempts, void(net::ConnectionAttempts*));
  MOCK_METHOD0(ClearConnectionAttempts, void());
  MOCK_METHOD1(AddConnectionAttempts, void(const net::ConnectionAttempts&));
  MOCK_CONST_METHOD0(GetTotalReceivedBytes, int64_t());

  // Methods specific to MockTCPClientSocket
  MOCK_METHOD1(Bind, int(const net::IPEndPoint&));
  MOCK_METHOD2(SetKeepAlive, bool(bool, int));
  MOCK_METHOD1(SetNoDelay, bool(bool));
};

MockTCPClientSocket::MockTCPClientSocket()
    : TCPClientSocket(net::AddressList(),
                      nullptr,
                      nullptr,
                      net::NetLogSource()) {}
MockTCPClientSocket::~MockTCPClientSocket() {}

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SOCKET_MOCK_TCP_CLIENT_SOCKET_H_
