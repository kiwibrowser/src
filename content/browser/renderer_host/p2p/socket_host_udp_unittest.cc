// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/p2p/socket_host_udp.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/logging.h"
#include "base/sys_byteorder.h"
#include "content/browser/renderer_host/p2p/socket_host_test_utils.h"
#include "content/browser/renderer_host/p2p/socket_host_throttler.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/datagram_server_socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DeleteArg;
using ::testing::DoAll;
using ::testing::Return;

namespace {

// TODO(nisse): We can't currently use rtc::ScopedFakeClock, because
// we don't link with webrtc rtc_base_tests_utils. So roll our own.

// Creating an object of this class makes rtc::TimeMicros() and
// related functions return zero unless the clock is advanced.
class ScopedFakeClock : public rtc::ClockInterface {
 public:
  ScopedFakeClock() { prev_clock_ = rtc::SetClockForTesting(this); }
  ~ScopedFakeClock() override { rtc::SetClockForTesting(prev_clock_); }
  // ClockInterface implementation.
  int64_t TimeNanos() const override { return time_nanos_; }
  void SetTimeNanos(uint64_t time_nanos) { time_nanos_ = time_nanos; }

 private:
  ClockInterface* prev_clock_;
  uint64_t time_nanos_ = 0;
};

class FakeDatagramServerSocket : public net::DatagramServerSocket {
 public:
  typedef std::pair<net::IPEndPoint, std::vector<char> > UDPPacket;

  // P2PSocketHostUdp destroys a socket on errors so sent packets
  // need to be stored outside of this object.
  FakeDatagramServerSocket(base::circular_deque<UDPPacket>* sent_packets,
                           std::vector<uint16_t>* used_ports)
      : sent_packets_(sent_packets),
        recv_address_(nullptr),
        recv_size_(0),
        used_ports_(used_ports) {}

  void Close() override {}

  int GetPeerAddress(net::IPEndPoint* address) const override {
    NOTREACHED();
    return net::ERR_SOCKET_NOT_CONNECTED;
  }

  int GetLocalAddress(net::IPEndPoint* address) const override {
    *address = address_;
    return 0;
  }

  void UseNonBlockingIO() override {}

  int Listen(const net::IPEndPoint& address) override {
    if (used_ports_) {
      for (auto used_port : *used_ports_) {
        if (used_port == address.port())
          return -1;
      }
      used_ports_->push_back(address.port());
    }

    address_ = address;
    return 0;
  }

  int RecvFrom(net::IOBuffer* buf,
               int buf_len,
               net::IPEndPoint* address,
               const net::CompletionCallback& callback) override {
    CHECK(recv_callback_.is_null());
    if (incoming_packets_.size() > 0) {
      scoped_refptr<net::IOBuffer> buffer(buf);
      int size = std::min(
          static_cast<int>(incoming_packets_.front().second.size()), buf_len);
      memcpy(buffer->data(), &*incoming_packets_.front().second.begin(), size);
      *address = incoming_packets_.front().first;
      incoming_packets_.pop_front();
      return size;
    } else {
      recv_callback_ = callback;
      recv_buffer_ = buf;
      recv_size_ = buf_len;
      recv_address_ = address;
      return net::ERR_IO_PENDING;
    }
  }

  int SendTo(net::IOBuffer* buf,
             int buf_len,
             const net::IPEndPoint& address,
             const net::CompletionCallback& callback) override {
    scoped_refptr<net::IOBuffer> buffer(buf);
    std::vector<char> data_vector(buffer->data(), buffer->data() + buf_len);
    sent_packets_->push_back(UDPPacket(address, data_vector));
    return buf_len;
  }

  int SetReceiveBufferSize(int32_t size) override { return net::OK; }

  int SetSendBufferSize(int32_t size) override { return net::OK; }

  int SetDoNotFragment() override { return net::OK; }

  void SetMsgConfirm(bool confirm) override {}

  void ReceivePacket(const net::IPEndPoint& address, std::vector<char> data) {
    if (!recv_callback_.is_null()) {
      int size = std::min(recv_size_, static_cast<int>(data.size()));
      memcpy(recv_buffer_->data(), &*data.begin(), size);
      *recv_address_ = address;
      net::CompletionCallback cb = recv_callback_;
      recv_callback_.Reset();
      recv_buffer_ = nullptr;
      std::move(cb).Run(size);
    } else {
      incoming_packets_.push_back(UDPPacket(address, data));
    }
  }

  const net::NetLogWithSource& NetLog() const override { return net_log_; }

  void AllowAddressReuse() override { NOTIMPLEMENTED(); }

  void AllowBroadcast() override { NOTIMPLEMENTED(); }

  int JoinGroup(const net::IPAddress& group_address) const override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int LeaveGroup(const net::IPAddress& group_address) const override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int SetMulticastInterface(uint32_t interface_index) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int SetMulticastTimeToLive(int time_to_live) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int SetMulticastLoopbackMode(bool loopback) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int SetDiffServCodePoint(net::DiffServCodePoint dscp) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  void DetachFromThread() override { NOTIMPLEMENTED(); }

 private:
  net::IPEndPoint address_;
  base::circular_deque<UDPPacket>* sent_packets_;
  base::circular_deque<UDPPacket> incoming_packets_;
  net::NetLogWithSource net_log_;

  scoped_refptr<net::IOBuffer> recv_buffer_;
  net::IPEndPoint* recv_address_;
  int recv_size_;
  net::CompletionCallback recv_callback_;
  std::vector<uint16_t>* used_ports_;
};

std::unique_ptr<net::DatagramServerSocket> CreateFakeDatagramServerSocket(
    base::circular_deque<FakeDatagramServerSocket::UDPPacket>* sent_packets,
    std::vector<uint16_t>* used_ports,
    net::NetLog* net_log) {
  return std::make_unique<FakeDatagramServerSocket>(sent_packets, used_ports);
}

}  // namespace

namespace content {

class P2PSocketHostUdpTest : public testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(
        sender_,
        Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
        .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

    socket_host_.reset(new P2PSocketHostUdp(
        &sender_, 0, &throttler_, /*net_log=*/nullptr,
        base::Bind(&CreateFakeDatagramServerSocket, &sent_packets_, nullptr)));

    local_address_ = ParseAddress(kTestLocalIpAddress, kTestPort1);
    socket_host_->Init(local_address_, 0, 0, P2PHostAndIPEndPoint());
    socket_ = GetSocketFromHost(socket_host_.get());

    dest1_ = ParseAddress(kTestIpAddress1, kTestPort1);
    dest2_ = ParseAddress(kTestIpAddress2, kTestPort2);
  }

  static FakeDatagramServerSocket* GetSocketFromHost(
      P2PSocketHostUdp* socket_host) {
    return static_cast<FakeDatagramServerSocket*>(socket_host->socket_.get());
  }

  P2PMessageThrottler throttler_;
  ScopedFakeClock fake_clock_;
  base::circular_deque<FakeDatagramServerSocket::UDPPacket> sent_packets_;
  FakeDatagramServerSocket* socket_;  // Owned by |socket_host_|.
  std::unique_ptr<P2PSocketHostUdp> socket_host_;
  MockIPCSender sender_;

  net::IPEndPoint local_address_;

  net::IPEndPoint dest1_;
  net::IPEndPoint dest2_;
};

// Verify that we can send STUN messages before we receive anything
// from the other side.
TEST_F(P2PSocketHostUdpTest, SendStunNoAuth) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  socket_host_->Send(dest1_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet2;
  CreateStunResponse(&packet2);
  socket_host_->Send(dest1_, packet2, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  std::vector<char> packet3;
  CreateStunError(&packet3);
  socket_host_->Send(dest1_, packet3, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  ASSERT_EQ(sent_packets_.size(), 3U);
  ASSERT_EQ(sent_packets_[0].second, packet1);
  ASSERT_EQ(sent_packets_[1].second, packet2);
  ASSERT_EQ(sent_packets_[2].second, packet3);
}

// Verify that no data packets can be sent before STUN binding has
// finished.
TEST_F(P2PSocketHostUdpTest, SendDataNoAuth) {
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  ASSERT_EQ(sent_packets_.size(), 0U);
}

// Verify that SetOption() doesn't crash after an error.
TEST_F(P2PSocketHostUdpTest, SetOptionAfterError) {
  // Get the sender into the error state.
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_host_->Send(dest1_, {1, 2, 3, 4}, rtc::PacketOptions(), 0,
                     TRAFFIC_ANNOTATION_FOR_TESTS);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Verify that SetOptions() fails, but doesn't crash.
  EXPECT_FALSE(socket_host_->SetOption(P2P_SOCKET_OPT_RCVBUF, 2048));
}

// Verify that we can send data after we've received STUN request
// from the other side.
TEST_F(P2PSocketHostUdpTest, SendAfterStunRequest) {
  // Receive packet from |dest1_|.
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->ReceivePacket(dest1_, request_packet);

  // Now we should be able to send any data to |dest1_|.
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  ASSERT_EQ(1U, sent_packets_.size());
  ASSERT_EQ(dest1_, sent_packets_[0].first);
}

// Verify that we can send data after we've received STUN response
// from the other side.
TEST_F(P2PSocketHostUdpTest, SendAfterStunResponse) {
  // Receive packet from |dest1_|.
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->ReceivePacket(dest1_, request_packet);

  // Now we should be able to send any data to |dest1_|.
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  ASSERT_EQ(1U, sent_packets_.size());
  ASSERT_EQ(dest1_, sent_packets_[0].first);
}

// Verify messages still cannot be sent to an unathorized host after
// successful binding with different host.
TEST_F(P2PSocketHostUdpTest, SendAfterStunResponseDifferentHost) {
  // Receive packet from |dest1_|.
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->ReceivePacket(dest1_, request_packet);

  // Should fail when trying to send the same packet to |dest2_|.
  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateRandomPacket(&packet);
  EXPECT_CALL(sender_,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
}

// Verify throttler not allowing unlimited sending of ICE messages to
// any destination.
TEST_F(P2PSocketHostUdpTest, ThrottleAfterLimit) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(3)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  throttler_.SetSendIceBandwidth(packet1.size() * 2);
  socket_host_->Send(dest1_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  net::IPEndPoint dest3 = ParseAddress(kTestIpAddress1, 2222);
  // This packet must be dropped by the throttler.
  socket_host_->Send(dest3, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  ASSERT_EQ(sent_packets_.size(), 2U);
}

// Verify we can send packets to a known destination when ICE throttling is
// active.
TEST_F(P2PSocketHostUdpTest, ThrottleAfterLimitAfterReceive) {
  // Receive packet from |dest1_|.
  std::vector<char> request_packet;
  CreateStunRequest(&request_packet);

  EXPECT_CALL(sender_, Send(MatchPacketMessage(request_packet)))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  socket_->ReceivePacket(dest1_, request_packet);

  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(6)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet1;
  CreateStunRequest(&packet1);
  throttler_.SetSendIceBandwidth(packet1.size());
  // |dest1_| is known address, throttling will not be applied.
  socket_host_->Send(dest1_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  // Trying to send the packet to dest1_ in the same window. It should go.
  socket_host_->Send(dest1_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  // Throttler should allow this packet to go through.
  socket_host_->Send(dest2_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);

  net::IPEndPoint dest3 = ParseAddress(kTestIpAddress1, 2223);
  // This packet will be dropped, as limit only for a single packet.
  socket_host_->Send(dest3, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  net::IPEndPoint dest4 = ParseAddress(kTestIpAddress1, 2224);
  // This packet should also be dropped.
  socket_host_->Send(dest4, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  // |dest1| is known, we can send as many packets to it.
  socket_host_->Send(dest1_, packet1, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  ASSERT_EQ(sent_packets_.size(), 4U);
}

// The fake clock mechanism used for this test doesn't work in component builds.
// See: https://bugs.chromium.org/p/webrtc/issues/detail?id=6490
#if defined(COMPONENT_BUILD)
#define MAYBE_ThrottlingStopsAtExpectedTimes DISABLED_ThrottlingStopsAtExpectedTimes
#else
#define MAYBE_ThrottlingStopsAtExpectedTimes ThrottlingStopsAtExpectedTimes
#endif
// Test that once the limit is hit, the throttling stops at the expected time,
// allowing packets to be sent again.
TEST_F(P2PSocketHostUdpTest, MAYBE_ThrottlingStopsAtExpectedTimes) {
  EXPECT_CALL(
      sender_,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSendComplete::ID))))
      .Times(12)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  rtc::PacketOptions options;
  std::vector<char> packet;
  CreateStunRequest(&packet);
  // Limit of 2 packets per second.
  throttler_.SetSendIceBandwidth(packet.size() * 2);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(2U, sent_packets_.size());

  // These packets must be dropped by the throttler since the limit was hit and
  // the time hasn't advanced.
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(2U, sent_packets_.size());

  // Advance the time to 0.999 seconds; throttling should still just barely be
  // active.
  fake_clock_.SetTimeNanos(rtc::kNumNanosecsPerMillisec * 999);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(2U, sent_packets_.size());

  // After hitting the second mark, we should be able to send again.
  // Add an extra millisecond to account for rounding errors.
  fake_clock_.SetTimeNanos(rtc::kNumNanosecsPerMillisec * 1001);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(3U, sent_packets_.size());

  // This time, hit the limit in the middle of the period.
  fake_clock_.SetTimeNanos(rtc::kNumNanosecsPerMillisec * 1500);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(4U, sent_packets_.size());

  // Again, throttling should be active until the next second mark.
  fake_clock_.SetTimeNanos(rtc::kNumNanosecsPerMillisec * 1999);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(4U, sent_packets_.size());
  fake_clock_.SetTimeNanos(rtc::kNumNanosecsPerMillisec * 2002);
  socket_host_->Send(dest1_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  socket_host_->Send(dest2_, packet, options, 0, TRAFFIC_ANNOTATION_FOR_TESTS);
  EXPECT_EQ(6U, sent_packets_.size());
}

// Verify that we can open UDP sockets listening in a given port range,
// and fail if all ports in the range are already in use.
TEST_F(P2PSocketHostUdpTest, PortRangeImplicitPort) {
  const uint16_t min_port = 10000;
  const uint16_t max_port = 10001;
  base::circular_deque<FakeDatagramServerSocket::UDPPacket> sent_packets;
  std::vector<uint16_t> used_ports;
  P2PSocketHostUdp::DatagramServerSocketFactory fake_socket_factory =
      base::Bind(&CreateFakeDatagramServerSocket, &sent_packets, &used_ports);
  P2PMessageThrottler throttler;
  MockIPCSender sender;
  EXPECT_CALL(
      sender,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
      .Times(max_port - min_port + 1)
      .WillRepeatedly(DoAll(DeleteArg<0>(), Return(true)));

  for (unsigned port = min_port; port <= max_port; ++port) {
    std::unique_ptr<P2PSocketHostUdp> socket_host(new P2PSocketHostUdp(
        &sender, 0, &throttler, /*net_log=*/nullptr, fake_socket_factory));
    net::IPEndPoint local_address = ParseAddress(kTestLocalIpAddress, 0);
    bool rv = socket_host->Init(local_address, min_port, max_port,
                                P2PHostAndIPEndPoint());
    EXPECT_TRUE(rv);

    FakeDatagramServerSocket* socket = GetSocketFromHost(socket_host.get());
    net::IPEndPoint bound_address;
    socket->GetLocalAddress(&bound_address);
    EXPECT_EQ(port, bound_address.port());
  }

  EXPECT_CALL(sender,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));
  std::unique_ptr<P2PSocketHostUdp> socket_host(
      new P2PSocketHostUdp(&sender, 0, &throttler, /*net_log=*/nullptr,
                           std::move(fake_socket_factory)));
  net::IPEndPoint local_address = ParseAddress(kTestLocalIpAddress, 0);
  bool rv = socket_host->Init(local_address, min_port, max_port,
                              P2PHostAndIPEndPoint());
  EXPECT_FALSE(rv);
}

// Verify that we can open a UDP socket listening in a given port included in
// a given valid range.
TEST_F(P2PSocketHostUdpTest, PortRangeExplictValidPort) {
  const uint16_t min_port = 10000;
  const uint16_t max_port = 10001;
  const uint16_t valid_port = min_port;
  base::circular_deque<FakeDatagramServerSocket::UDPPacket> sent_packets;
  std::vector<uint16_t> used_ports;
  P2PSocketHostUdp::DatagramServerSocketFactory fake_socket_factory =
      base::Bind(&CreateFakeDatagramServerSocket, &sent_packets, &used_ports);
  P2PMessageThrottler throttler;
  MockIPCSender sender;
  EXPECT_CALL(
      sender,
      Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnSocketCreated::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  std::unique_ptr<P2PSocketHostUdp> socket_host(
      new P2PSocketHostUdp(&sender, 0, &throttler, /*net_log=*/nullptr,
                           std::move(fake_socket_factory)));
  net::IPEndPoint local_address = ParseAddress(kTestLocalIpAddress, valid_port);
  bool rv = socket_host->Init(local_address, min_port, max_port,
                              P2PHostAndIPEndPoint());
  EXPECT_TRUE(rv);

  FakeDatagramServerSocket* socket = GetSocketFromHost(socket_host.get());
  net::IPEndPoint bound_address;
  socket->GetLocalAddress(&bound_address);
  EXPECT_EQ(local_address.port(), bound_address.port());
}

// Verify that we cannot open a UDP socket listening in a given port not
// included in a given valid range.
TEST_F(P2PSocketHostUdpTest, PortRangeExplictInvalidPort) {
  const uint16_t min_port = 10000;
  const uint16_t max_port = 10001;
  const uint16_t invalid_port = max_port + 1;
  base::circular_deque<FakeDatagramServerSocket::UDPPacket> sent_packets;
  std::vector<uint16_t> used_ports;
  P2PSocketHostUdp::DatagramServerSocketFactory fake_socket_factory =
      base::Bind(&CreateFakeDatagramServerSocket, &sent_packets, &used_ports);
  P2PMessageThrottler throttler;
  MockIPCSender sender;
  EXPECT_CALL(sender,
              Send(MatchMessage(static_cast<uint32_t>(P2PMsg_OnError::ID))))
      .WillOnce(DoAll(DeleteArg<0>(), Return(true)));

  std::unique_ptr<P2PSocketHostUdp> socket_host(
      new P2PSocketHostUdp(&sender, 0, &throttler, /*net_log=*/nullptr,
                           std::move(fake_socket_factory)));
  net::IPEndPoint local_address =
      ParseAddress(kTestLocalIpAddress, invalid_port);
  bool rv = socket_host->Init(local_address, min_port, max_port,
                              P2PHostAndIPEndPoint());
  EXPECT_FALSE(rv);
}

}  // namespace content
