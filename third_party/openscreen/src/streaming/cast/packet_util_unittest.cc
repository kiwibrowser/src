// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/packet_util.h"

#include "absl/types/span.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

// Tests that a simple RTCP packet containing only a Sender Report can be
// identified.
TEST(PacketUtilTest, InspectsRtcpPacketFromSender) {
  // clang-format off
  const uint8_t kSenderReportPacket[] = {
    0b10000000,  // Version=2, Padding=no, ItemCount=0.
    200,  // RTCP Packet type.
    0x00, 0x06,  // Length of remainder of packet, in 32-bit words.
    1, 2, 3, 4,  // SSRC of sender.
    0xe0, 0x73, 0x2e, 0x54,  // NTP Timestamp (late evening on 2019-04-30).
        0x80, 0x00, 0x00, 0x00,
    0x00, 0x14, 0x99, 0x70,  // RTP Timestamp (15 seconds, 90kHz timebase).
    0x00, 0x00, 0x01, 0xff,  // Sender's Packet Count.
    0x00, 0x07, 0x11, 0x0d,  // Sender's Octet Count.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x01020304;

  const auto result = InspectPacketForRouting(kSenderReportPacket);
  EXPECT_EQ(ApparentPacketType::RTCP, result.first);
  EXPECT_EQ(kSenderSsrc, result.second);
}

// Tests that compound RTCP packets containing a Receiver Report and/or a Cast
// Feedback message can be identified.
TEST(PacketUtilTest, InspectsRtcpPacketFromReceiver) {
  // clang-format off
  const uint8_t kReceiverReportPacket[] = {
    0b10000001,  // Version=2, Padding=no, ItemCount=1.
    201,  // RTCP Packet type.
    0x00, 0x01,  // Length of remainder of packet, in 32-bit words.
    9, 8, 7, 6,  // SSRC of receiver.
  };
  const uint8_t kCastFeedbackPacket[] = {
    // Cast Feedback
    0b10000000 | 15,  // Version=2, Padding=no, Subtype=15.
    206,  // RTCP Packet type byte.
    0x00, 0x04,  // Length of remainder of packet, in 32-bit words.
    9, 8, 7, 6,  // SSRC of receiver.
    1, 2, 3, 4,  // SSRC of sender.
    'C', 'A', 'S', 'T',
    0x0a,  // Checkpoint Frame ID (lower 8 bits).
    0x00,  // Number of "Loss Fields"
    0x00, 0x28,  // Current Playout Delay in milliseconds.
  };
  // clang-format on
  const Ssrc kReceiverSsrc = 0x09080706;

  {
    const auto result = InspectPacketForRouting(kReceiverReportPacket);
    EXPECT_EQ(ApparentPacketType::RTCP, result.first);
    EXPECT_EQ(kReceiverSsrc, result.second);
  }

  {
    const auto result = InspectPacketForRouting(kCastFeedbackPacket);
    EXPECT_EQ(ApparentPacketType::RTCP, result.first);
    EXPECT_EQ(kReceiverSsrc, result.second);
  }

  const absl::Span<const uint8_t> kCompoundCombinations[2][2] = {
      {kReceiverReportPacket, kCastFeedbackPacket},
      {kCastFeedbackPacket, kReceiverReportPacket},
  };
  for (const auto& combo : kCompoundCombinations) {
    uint8_t compound_packet[sizeof(kReceiverReportPacket) +
                            sizeof(kCastFeedbackPacket)];
    memcpy(compound_packet, combo[0].data(), combo[0].size());
    memcpy(compound_packet + combo[0].size(), combo[1].data(), combo[1].size());

    const auto result = InspectPacketForRouting(compound_packet);
    EXPECT_EQ(ApparentPacketType::RTCP, result.first);
    EXPECT_EQ(kReceiverSsrc, result.second);
  }
}

// Tests that a RTP packet can be identified.
TEST(PacketUtilTest, InspectsRtpPacket) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xbe, 0xef,  // Sequence number.
    9, 8, 7, 6,  // RTP timestamp.
    1, 2, 3, 4,  // SSRC.
    0b10000000,  // Is key frame, no extensions.
    5,  // Frame ID.
    0xa, 0xb,  // Packet ID.
    0xa, 0xc,  // Max packet ID.
    0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x01020304;

  const auto result = InspectPacketForRouting(kInput);
  EXPECT_EQ(ApparentPacketType::RTP, result.first);
  EXPECT_EQ(kSenderSsrc, result.second);
}

// Tests that a malformed RTP packet can be identified.
TEST(PacketUtilTest, InspectsMalformedRtpPacket) {
  // clang-format off
  const uint8_t kInput[] = {
    0b11000000,  // BAD: Version/Padding byte.
    96,  // Payload type byte.
    0xbe, 0xef,  // Sequence number.
    9, 8, 7, 6,  // RTP timestamp.
    1, 2, 3, 4,  // SSRC.
    0b10000000,  // Is key frame, no extensions.
    5,  // Frame ID.
    0xa, 0xb,  // Packet ID.
    0xa, 0xc,  // Max packet ID.
    0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,  // Payload.
  };
  // clang-format on

  const auto result = InspectPacketForRouting(kInput);
  EXPECT_EQ(ApparentPacketType::UNKNOWN, result.first);
}

// Tests that an empty packet is classified as unknown.
TEST(PacketUtilTest, InspectsEmptyPacket) {
  const uint8_t kInput[] = {};

  const auto result =
      InspectPacketForRouting(absl::Span<const uint8_t>(kInput, 0));
  EXPECT_EQ(ApparentPacketType::UNKNOWN, result.first);
}

// Tests that a packet with garbage is classified as unknown.
TEST(PacketUtilTest, InspectsGarbagePacket) {
  // clang-format off
  const uint8_t kInput[] = {
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
  };
  // clang-format on

  const auto result = InspectPacketForRouting(kInput);
  EXPECT_EQ(ApparentPacketType::UNKNOWN, result.first);
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
