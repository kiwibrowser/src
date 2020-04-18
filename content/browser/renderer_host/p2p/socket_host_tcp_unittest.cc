// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/p2p/socket_host_tcp.h"

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sys_byteorder.h"
#include "base/test/scoped_task_environment.h"
#include "content/browser/renderer_host/p2p/socket_host_test_utils.h"
#include "jingle/glue/fake_ssl_client_socket.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/stream_socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/proxy_resolving_client_socket_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DeleteArg;
using ::testing::DoAll;
using ::testing::Return;

namespace content {

class P2PSocketHostTcpTestBase : public testing::Test {
 protected:
  explicit P2PSocketHostTcpTestBase(P2PSocketType type)
      : socket_type_(type) {
  }

  void SetUp() override {
    EXPECT_CALL(
        sender_,
        Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
        .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

    if (socket_type_ == P2P_SOCKET_TCP_CLIENT) {
      socket_host_.reset(new P2PSocketHostTcp(
          &sender_, 0, P2P_SOCKET_TCP_CLIENT, nullptr, nullptr));
    } else {
      socket_host_.reset(new P2PSocketHostStunTcp(
          &sender_, 0, P2P_SOCKET_STUN_TCP_CLIENT, nullptr, nullptr));
    }

    socket_ = new FakeSocket(&sent_data_);
    socket_->SetLocalAddress(ParseAddress(kTestLocalIpAddress, kTestPort1));
    socket_host_->socket_.reset(socket_);

    dest_.ip_address = ParseAddress(kTestIpAddress1, kTestPort1);

    local_address_ = ParseAddress(kTestLocalIpAddress, kTestPort1);

    socket_host_->remote_address_ = dest_;
    socket_host_->state_ = P2PSocketHost::STATE_CONNECTING;
    socket_host_->OnConnected(net::OK);
  }

  std::string IntToSize(int size) {
    std::string result;
    uint16_t size16 = base::HostToNet16(size);
    result.resize(sizeof(size16));
    memcpy(&result[0], &size16, sizeof(size16));
    return result;
  }

  std::string sent_data_;
  FakeSocket* socket_;  // Owned by |socket_host_|.
  std::unique_ptr<P2PSocketHostTcpBase> socket_host_;
  MockIPCSender sender_;

  net::IPEndPoint local_address_;
  P2PHostAndIPEndPoint dest_;
  P2PSocketType socket_type_;
};

class P2PSocketHostTcpTest : public P2PSocketHostTcpTestBase {
 protected:
  P2PSocketHostTcpTest() : P2PSocketHostTcpTestBase(P2P_SOCKET_TCP_CLIENT) { }
};

class P2PSocketHostStunTcpTest : public P2PSocketHostTcpTestBase {
 protected:
  P2PSocketHostStunTcpTest()
      : P2PSocketHostTcpTestBase(P2P_SOCKET_STUN_TCP_CLIENT) {
  }
};

// Verify that we can send STUN message and that they are formatted
// properly.
TEST_F(P2PSocketHostTcpTest, SendStunNoAuth) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet3;
  CreateStunError(&packet3);
  socket_host_->Send(dest_.ip_address, packet3, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string expected_data;
  expected_data.append(IntToSize(packet1.size()));
  expected_data.append(packet1.begin(), packet1.end());
  expected_data.append(IntToSize(packet2.size()));
  expected_data.append(packet2.begin(), packet2.end());
  expected_data.append(IntToSize(packet3.size()));
  expected_data.append(packet3.begin(), packet3.end());

  EXPECT_EQ(expected_data, sent_data_);
}

// Verify that we can receive STUN messages from the socket, and that
// the messages are parsed properly.
TEST_F(P2PSocketHostTcpTest, ReceiveStun) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet3;
  CreateStunError(&packet3);
  socket_host_->Send(dest_.ip_address, packet3, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string received_data;
  received_data.append(IntToSize(packet1.size()));
  received_data.append(packet1.begin(), packet1.end());
  received_data.append(IntToSize(packet2.size()));
  received_data.append(packet2.begin(), packet2.end());
  received_data.append(IntToSize(packet3.size()));
  received_data.append(packet3.begin(), packet3.end());

  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet1)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet2)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet3)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  size_t pos = 0;
  size_t step_sizes[] = {3, 2, 1};
  size_t step = 0;
  while (pos < received_data.size()) {
    size_t step_size = std::min(step_sizes[step], received_data.size() - pos);
    socket_->AppendInputData(&received_data[pos], step_size);
    pos += step_size;
    if (++step >= arraysize(step_sizes))
      step = 0;
  }
}

// Verify that we can't send data before we've received STUN response
// from the other side.
TEST_F(P2PSocketHostTcpTest, SendDataNoAuth) {
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest_.ip_address, packet, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  EXPECT_EQ(0U, sent_data_.size());
}

// Verify that SetOption() doesn't crash after an error.
TEST_F(P2PSocketHostTcpTest, SetOptionAfterError) {
  // Get the sender into the error state.
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_host_->Send(dest_.ip_address, {1, 2, 3, 4}, rtc::PacketOptions(), 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Verify that SetOptions() fails, but doesn't crash.
  EXPECT_FALSE(socket_host_->SetOption(P2P_SOCKET_OPT_RCVBUF, 2048));
}

// Verify that we can send data after we've received STUN response
// from the other side.
TEST_F(P2PSocketHostTcpTest, SendAfterStunRequest) {
  // Receive packet from |dest_|.
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  std::string received_data;
  received_data.append(IntToSize(request_packet.size()));
  received_data.append(request_packet.begin(), request_packet.end());

  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->AppendInputData(&received_data[0], received_data.size());

  rtc::PacketOptions options;
  // Now we should be able to send any data to |dest_|.
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest_.ip_address, packet, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string expected_data;
  expected_data.append(IntToSize(packet.size()));
  expected_data.append(packet.begin(), packet.end());

  EXPECT_EQ(expected_data, sent_data_);
}

// Verify that asynchronous writes are handled correctly.
TEST_F(P2PSocketHostTcpTest, AsyncWrites) {
  base::MessageLoop message_loop;

  socket_->set_async_write(true);

  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(2)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);

  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  base::RunLoop().RunUntilIdle();

  std::string expected_data;
  expected_data.append(IntToSize(packet1.size()));
  expected_data.append(packet1.begin(), packet1.end());
  expected_data.append(IntToSize(packet2.size()));
  expected_data.append(packet2.begin(), packet2.end());

  EXPECT_EQ(expected_data, sent_data_);
}

TEST_F(P2PSocketHostTcpTest, PacketIdIsPropagated) {
  base::MessageLoop message_loop;

  socket_->set_async_write(true);

  const int32_t kRtcPacketId = 1234;

  base::TimeTicks now = base::TimeTicks::Now();

  EXPECT_CALL(sender_, Send(MatchSendPacketMetrics(kRtcPacketId, now)))
      .Times(1)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  options.packet_id = kRtcPacketId;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);

  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  base::RunLoop().RunUntilIdle();

  std::string expected_data;
  expected_data.append(IntToSize(packet1.size()));
  expected_data.append(packet1.begin(), packet1.end());

  EXPECT_EQ(expected_data, sent_data_);
}

TEST_F(P2PSocketHostTcpTest, SendDataWithPacketOptions) {
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  std::string received_data;
  received_data.append(IntToSize(request_packet.size()));
  received_data.append(request_packet.begin(), request_packet.end());

  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->AppendInputData(&received_data[0], received_data.size());

  rtc::PacketOptions options;
  options.packet_time_params.rtp_sendtime_extension_id = 3;
  // Now we should be able to send any data to |dest_|.
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  // Make it a RTP packet.
  *reinterpret_cast<uint16_t*>(&*packet.begin()) = base::HostToNet16(0x8000);
  socket_host_->Send(dest_.ip_address, packet, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string expected_data;
  expected_data.append(IntToSize(packet.size()));
  expected_data.append(packet.begin(), packet.end());

  EXPECT_EQ(expected_data, sent_data_);
}

// Verify that we can send STUN message and that they are formatted
// properly.
TEST_F(P2PSocketHostStunTcpTest, SendStunNoAuth) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet3;
  CreateStunError(&packet3);
  socket_host_->Send(dest_.ip_address, packet3, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string expected_data;
  expected_data.append(packet1.begin(), packet1.end());
  expected_data.append(packet2.begin(), packet2.end());
  expected_data.append(packet3.begin(), packet3.end());

  EXPECT_EQ(expected_data, sent_data_);
}

// Verify that we can receive STUN messages from the socket, and that
// the messages are parsed properly.
TEST_F(P2PSocketHostStunTcpTest, ReceiveStun) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet3;
  CreateStunError(&packet3);
  socket_host_->Send(dest_.ip_address, packet3, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::string received_data;
  received_data.append(packet1.begin(), packet1.end());
  received_data.append(packet2.begin(), packet2.end());
  received_data.append(packet3.begin(), packet3.end());

  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet1)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet2)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  EXPECT_CALL(sender_, Send(MatchPacketMessage(packet3)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  size_t pos = 0;
  size_t step_sizes[] = {3, 2, 1};
  size_t step = 0;
  while (pos < received_data.size()) {
    size_t step_size = std::min(step_sizes[step], received_data.size() - pos);
    socket_->AppendInputData(&received_data[pos], step_size);
    pos += step_size;
    if (++step >= arraysize(step_sizes))
      step = 0;
  }
}

// Verify that we can't send data before we've received STUN response
// from the other side.
TEST_F(P2PSocketHostStunTcpTest, SendDataNoAuth) {
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest_.ip_address, packet, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  EXPECT_EQ(0U, sent_data_.size());
}

// Verify that asynchronous writes are handled correctly.
TEST_F(P2PSocketHostStunTcpTest, AsyncWrites) {
  base::MessageLoop message_loop;

  socket_->set_async_write(true);

  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(2)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest_.ip_address, packet1, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest_.ip_address, packet2, options, 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);

  base::RunLoop().RunUntilIdle();

  std::string expected_data;
  expected_data.append(packet1.begin(), packet1.end());
  expected_data.append(packet2.begin(), packet2.end());

  EXPECT_EQ(expected_data, sent_data_);
}

// When pseudo-tls is used (e.g. for P2P_SOCKET_SSLTCP_CLIENT),
// network::ProxyResolvingClientSocket::Connect() won't be called twice.
// Regression test for crbug.com/840797.
TEST(P2PSocketHostTcpWithPseudoTlsTest, Basic) {
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);
  MockIPCSender sender;
  EXPECT_CALL(
      sender,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  net::TestURLRequestContext context(true);
  net::MockClientSocketFactory mock_socket_factory;
  context.set_client_socket_factory(&mock_socket_factory);
  context.Init();
  network::ProxyResolvingClientSocketFactory factory(&mock_socket_factory,
                                                     &context);

  base::StringPiece ssl_client_hello =
      jingle_glue::FakeSSLClientSocket::GetSslClientHello();
  base::StringPiece ssl_server_hello =
      jingle_glue::FakeSSLClientSocket::GetSslServerHello();
  net::MockRead reads[] = {
      net::MockRead(net::ASYNC, ssl_server_hello.data(),
                    ssl_server_hello.size()),
      net::MockRead(net::SYNCHRONOUS, net::ERR_IO_PENDING)};
  net::MockWrite writes[] = {net::MockWrite(
      net::SYNCHRONOUS, ssl_client_hello.data(), ssl_client_hello.size())};
  net::StaticSocketDataProvider data_provider(reads, writes);
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  data_provider.set_connect_data(
      net::MockConnect(net::SYNCHRONOUS, net::OK, server_addr));
  mock_socket_factory.AddSocketDataProvider(&data_provider);

  P2PSocketHostTcp host(&sender, 0 /*socket_id*/, P2P_SOCKET_SSLTCP_CLIENT,
                        nullptr, &factory);
  P2PHostAndIPEndPoint dest;
  dest.ip_address = server_addr;
  bool success = host.Init(net::IPEndPoint(net::IPAddress::IPv4Localhost(), 0),
                           0, 0, dest);
  EXPECT_TRUE(success);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
}

class P2PSocketHostTcpWithTlsTest
    : public testing::TestWithParam<std::tuple<net::IoMode, P2PSocketType>> {};

INSTANTIATE_TEST_CASE_P(
    /* no prefix */,
    P2PSocketHostTcpWithTlsTest,
    ::testing::Combine(::testing::Values(net::SYNCHRONOUS, net::ASYNC),
                       ::testing::Values(P2P_SOCKET_TLS_CLIENT,
                                         P2P_SOCKET_STUN_TLS_CLIENT)));

// Tests that if a socket type satisfies IsTlsClientSocket(), TLS connection is
// established.
TEST_P(P2PSocketHostTcpWithTlsTest, Basic) {
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);
  MockIPCSender sender;
  EXPECT_CALL(
      sender,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  net::TestURLRequestContext context(true);
  net::MockClientSocketFactory mock_socket_factory;
  context.set_client_socket_factory(&mock_socket_factory);
  context.Init();
  network::ProxyResolvingClientSocketFactory factory(&mock_socket_factory,
                                                     &context);
  const net::IoMode io_mode = std::get<0>(GetParam());
  const P2PSocketType socket_type = std::get<1>(GetParam());
  // OnOpen() calls DoRead(), so populate the mock socket with a pending read.
  net::MockRead reads[] = {
      net::MockRead(net::SYNCHRONOUS, net::ERR_IO_PENDING)};
  net::StaticSocketDataProvider data_provider(
      reads, base::span<const net::MockWrite>());
  net::IPEndPoint server_addr(net::IPAddress::IPv4Localhost(), 1234);
  data_provider.set_connect_data(
      net::MockConnect(io_mode, net::OK, server_addr));
  net::SSLSocketDataProvider ssl_socket_provider(io_mode, net::OK);
  mock_socket_factory.AddSocketDataProvider(&data_provider);
  mock_socket_factory.AddSSLSocketDataProvider(&ssl_socket_provider);

  std::unique_ptr<P2PSocketHostTcpBase> host;
  if (socket_type == P2P_SOCKET_STUN_TLS_CLIENT) {
    host = std::make_unique<P2PSocketHostStunTcp>(
        &sender, 0 /*socket_id*/, socket_type, nullptr, &factory);
  } else {
    host = std::make_unique<P2PSocketHostTcp>(&sender, 0 /*socket_id*/,
                                              socket_type, nullptr, &factory);
  }
  P2PHostAndIPEndPoint dest;
  dest.ip_address = server_addr;
  bool success = host->Init(net::IPEndPoint(net::IPAddress::IPv4Localhost(), 0),
                            0, 0, dest);
  EXPECT_TRUE(success);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(data_provider.AllReadDataConsumed());
  EXPECT_TRUE(data_provider.AllWriteDataConsumed());
  EXPECT_TRUE(ssl_socket_provider.ConnectDataConsumed());
}

}  // namespace content
