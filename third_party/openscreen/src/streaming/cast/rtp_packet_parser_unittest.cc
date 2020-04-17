// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_packet_parser.h"

#include "streaming/cast/rtp_defines.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

// Tests that a simple packet for a key frame can be parsed.
TEST(RtpPacketParserTest, ParsesPacketForKeyFrame) {
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

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_TRUE(result);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, result->payload_type);
  EXPECT_EQ(UINT16_C(0xbeef), result->sequence_number);
  EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(0x09080706),
            result->rtp_timestamp);
  EXPECT_TRUE(result->is_key_frame);
  EXPECT_EQ(FrameId::first() + 5, result->frame_id);
  EXPECT_EQ(FramePacketId{0x0a0b}, result->packet_id);
  EXPECT_EQ(FramePacketId{0x0a0c}, result->max_packet_id);
  EXPECT_EQ(FrameId::first() + 5, result->referenced_frame_id);
  EXPECT_EQ(0, result->new_playout_delay.count());
  const absl::Span<const uint8_t> expected_payload(kInput + 18, 8);
  ASSERT_EQ(expected_payload, result->payload);
  EXPECT_TRUE(expected_payload == result->payload);
}

// Tests that a packet which includes a "referenced frame ID" can be parsed.
TEST(RtpPacketParserTest, ParsesPacketForNonKeyFrameWithReferenceFrameId) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xde, 0xad,  // Sequence number.
    2, 4, 6, 8,  // RTP timestamp.
    0, 0, 1, 1,  // SSRC.
    0b01000000,  // Not a key frame, but has ref frame ID; no extensions.
    42,  // Frame ID.
    0x0, 0xb,  // Packet ID.
    0x0, 0xc,  // Max packet ID.
    39,  // Reference Frame ID.
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x00000101;

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_TRUE(result);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, result->payload_type);
  EXPECT_EQ(UINT16_C(0xdead), result->sequence_number);
  EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(0x02040608),
            result->rtp_timestamp);
  EXPECT_FALSE(result->is_key_frame);
  EXPECT_EQ(FrameId::first() + 42, result->frame_id);
  EXPECT_EQ(FramePacketId{0x000b}, result->packet_id);
  EXPECT_EQ(FramePacketId{0x000c}, result->max_packet_id);
  EXPECT_EQ(FrameId::first() + 39, result->referenced_frame_id);
  EXPECT_EQ(0, result->new_playout_delay.count());
  const absl::Span<const uint8_t> expected_payload(kInput + 19, 15);
  ASSERT_EQ(expected_payload, result->payload);
  EXPECT_TRUE(expected_payload == result->payload);
}

// Tests that a packet which lacks a "referenced frame ID" field can be parsed,
// but the parser will provide the implied referenced_frame_id value in the
// result.
TEST(RtpPacketParserTest, ParsesPacketForNonKeyFrameWithoutReferenceFrameId) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xde, 0xad,  // Sequence number.
    2, 4, 6, 8,  // RTP timestamp.
    0, 0, 1, 1,  // SSRC.
    0b00000000,  // Not a key frame, no ref frame ID; no extensions.
    42,  // Frame ID.
    0x0, 0xb,  // Packet ID.
    0x0, 0xc,  // Max packet ID.
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x00000101;

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_TRUE(result);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, result->payload_type);
  EXPECT_EQ(UINT16_C(0xdead), result->sequence_number);
  EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(0x02040608),
            result->rtp_timestamp);
  EXPECT_FALSE(result->is_key_frame);
  EXPECT_EQ(FrameId::first() + 42, result->frame_id);
  EXPECT_EQ(FramePacketId{0x000b}, result->packet_id);
  EXPECT_EQ(FramePacketId{0x000c}, result->max_packet_id);
  EXPECT_EQ(FrameId::first() + 41, result->referenced_frame_id);
  EXPECT_EQ(0, result->new_playout_delay.count());
  const absl::Span<const uint8_t> expected_payload(kInput + 18, 15);
  ASSERT_EQ(expected_payload, result->payload);
  EXPECT_TRUE(expected_payload == result->payload);
}

// Tests that a packet indicating a new playout delay can be parsed.
TEST(RtpPacketParserTest, ParsesPacketWithAdaptiveLatencyExtension) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xde, 0xad,  // Sequence number.
    2, 4, 6, 8,  // RTP timestamp.
    0, 0, 1, 1,  // SSRC.
    0b11000001,  // Is key frame, has ref frame ID; has one extension.
    64,  // Frame ID.
    0x0, 0x0,  // Packet ID.
    0x0, 0xc,  // Max packet ID.
    64,  // Reference Frame ID.
    4, 2, 1, 14,  // Cast Adaptive Latency Extension data.
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x00000101;

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_TRUE(result);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, result->payload_type);
  EXPECT_EQ(UINT16_C(0xdead), result->sequence_number);
  EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(0x02040608),
            result->rtp_timestamp);
  EXPECT_TRUE(result->is_key_frame);
  EXPECT_EQ(FrameId::first() + 64, result->frame_id);
  EXPECT_EQ(FramePacketId{0x0000}, result->packet_id);
  EXPECT_EQ(FramePacketId{0x000c}, result->max_packet_id);
  EXPECT_EQ(FrameId::first() + 64, result->referenced_frame_id);
  EXPECT_EQ(270, result->new_playout_delay.count());
  const absl::Span<const uint8_t> expected_payload(kInput + 23, 15);
  ASSERT_EQ(expected_payload, result->payload);
  EXPECT_TRUE(expected_payload == result->payload);
}

// Tests that the parser can handle multiple Cast Header Extensions in a RTP
// packet, and ignores all but the one (Adaptive Latency) that it understands.
TEST(RtpPacketParserTest, ParsesPacketWithMultipleExtensions) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xde, 0xad,  // Sequence number.
    2, 4, 6, 8,  // RTP timestamp.
    0, 0, 1, 1,  // SSRC.
    0b11000011,  // Is key frame, has ref frame ID; has 3 extensions.
    64,  // Frame ID.
    0x0, 0xb,  // Packet ID.
    0x0, 0xc,  // Max packet ID.
    64,  // Reference Frame ID.
    8, 2, 0, 0,  // Unknown extension with 2 bytes of data.
    4, 2, 1, 14,  // Cast Adaptive Latency Extension data.
    16, 5, 0, 0, 0, 0, 0,  // Unknown extension with 5 bytes of data.
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x00000101;

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_TRUE(result);
  EXPECT_EQ(RtpPayloadType::kAudioOpus, result->payload_type);
  EXPECT_EQ(UINT16_C(0xdead), result->sequence_number);
  EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(0x02040608),
            result->rtp_timestamp);
  EXPECT_TRUE(result->is_key_frame);
  EXPECT_EQ(FrameId::first() + 64, result->frame_id);
  EXPECT_EQ(FramePacketId{0x000b}, result->packet_id);
  EXPECT_EQ(FramePacketId{0x000c}, result->max_packet_id);
  EXPECT_EQ(FrameId::first() + 64, result->referenced_frame_id);
  EXPECT_EQ(270, result->new_playout_delay.count());
  const absl::Span<const uint8_t> expected_payload(kInput + 34, 15);
  ASSERT_EQ(expected_payload, result->payload);
  EXPECT_TRUE(expected_payload == result->payload);
}

// Tests that the parser ignores packets from an unknown source.
TEST(RtpPacketParserTest, IgnoresPacketWithWrongSsrc) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xbe, 0xef,  // Sequence number.
    9, 8, 7, 6,  // RTP timestamp.
    4, 3, 2, 1,  // SSRC.
    0b10000000,  // Is key frame, no extensions.
    5,  // Frame ID.
    0xa, 0xb,  // Packet ID.
    0xa, 0xc,  // Max packet ID.
    0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x01020304;

  RtpPacketParser parser(kSenderSsrc);
  const auto result = parser.Parse(kInput);
  ASSERT_FALSE(result);
}

// Tests that unexpected truncations in the RTP packets does not crash the
// parser, and that it correctly errors-out.
TEST(RtpPacketParserTest, RejectsTruncatedPackets) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xde, 0xad,  // Sequence number.
    2, 4, 6, 8,  // RTP timestamp.
    0, 0, 1, 1,  // SSRC.
    0b11000011,  // Is key frame, has ref frame ID; has 3 extensions.
    64,  // Frame ID.
    0x0, 0xb,  // Packet ID.
    0x0, 0xc,  // Max packet ID.
    64,  // Reference Frame ID.
    8, 2, 0, 0,  // Unknown extension with 2 bytes of data.
    4, 2, 1, 14,  // Cast Adaptive Latency Extension data.
    16, 5, 0, 0, 0, 0, 0,  // Unknown extension with 5 bytes of data.
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x00000101;

  RtpPacketParser parser(kSenderSsrc);
  ASSERT_FALSE(parser.Parse(absl::Span<const uint8_t>(kInput, 1)));
  ASSERT_FALSE(parser.Parse(absl::Span<const uint8_t>(kInput, 18)));
  ASSERT_FALSE(parser.Parse(absl::Span<const uint8_t>(kInput, 22)));
  ASSERT_FALSE(parser.Parse(absl::Span<const uint8_t>(kInput, 33)));

  // When truncated to 34 bytes, the parser should see it as a packet with zero
  // payload bytes.
  const auto result_without_payload =
      parser.Parse(absl::Span<const uint8_t>(kInput, 34));
  ASSERT_TRUE(result_without_payload);
  EXPECT_TRUE(result_without_payload->payload.empty());

  // And, of course, with the entire kInput available, the parser should see it
  // as a packet with 15 bytes of payload.
  const auto result_with_payload =
      parser.Parse(absl::Span<const uint8_t>(kInput, sizeof(kInput)));
  ASSERT_TRUE(result_with_payload);
  EXPECT_EQ(size_t{15}, result_with_payload->payload.size());
}

// Tests that the parser rejects invalid packet ID values.
TEST(RtpPacketParserTest, RejectsPacketWithBadFramePacketId) {
  // clang-format off
  const uint8_t kInput[] = {
    0b10000000,  // Version/Padding byte.
    96,  // Payload type byte.
    0xbe, 0xef,  // Sequence number.
    9, 8, 7, 6,  // RTP timestamp.
    1, 2, 3, 4,  // SSRC.
    0b10000000,  // Is key frame, no extensions.
    5,  // Frame ID.
    0xa, 0xb,  // Packet ID (which is GREATER than the max packet ID).
    0x0, 0x1,  // Max packet ID.
    0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,  // Payload.
  };
  // clang-format on
  const Ssrc kSenderSsrc = 0x01020304;

  RtpPacketParser parser(kSenderSsrc);
  ASSERT_FALSE(parser.Parse(kInput));
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
