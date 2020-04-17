// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtcp_common.h"

#include "absl/types/span.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

// Tests that the RTCP Common Header for a packet type that includes an Item
// Count is successfully serialized and re-parsed.
TEST(RtcpCommonTest, SerializesAndParsesHeaderForSenderReports) {
  RtcpCommonHeader original;
  original.packet_type = RtcpPacketType::kSenderReport;
  original.with.report_count = 31;
  original.payload_size = 16;

  uint8_t buffer[kRtcpCommonHeaderSize];
  original.Serialize(buffer);

  const auto parsed = RtcpCommonHeader::Parse(buffer);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.packet_type, parsed->packet_type);
  EXPECT_EQ(original.with.report_count, parsed->with.report_count);
  EXPECT_EQ(original.payload_size, parsed->payload_size);
}

// Tests that the RTCP Common Header for a packet type that includes a RTCP
// Subtype is successfully serialized and re-parsed.
TEST(RtcpCommonTest, SerializesAndParsesHeaderForCastFeedback) {
  RtcpCommonHeader original;
  original.packet_type = RtcpPacketType::kPayloadSpecific;
  original.with.subtype = RtcpSubtype::kFeedback;
  original.payload_size = 99 * sizeof(uint32_t);

  uint8_t buffer[kRtcpCommonHeaderSize];
  original.Serialize(buffer);

  const auto parsed = RtcpCommonHeader::Parse(buffer);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.packet_type, parsed->packet_type);
  EXPECT_EQ(original.with.subtype, parsed->with.subtype);
  EXPECT_EQ(original.payload_size, parsed->payload_size);
}

// Tests that a RTCP Common Header will not be parsed from an empty buffer.
TEST(RtcpCommonTest, WillNotParseHeaderFromEmptyBuffer) {
  const uint8_t kEmptyPacket[] = {};
  EXPECT_FALSE(
      RtcpCommonHeader::Parse(absl::Span<const uint8_t>(kEmptyPacket, 0))
          .has_value());
}

// Tests that a RTCP Common Header will not be parsed from a buffer containing
// garbage data.
TEST(RtcpCommonTest, WillNotParseHeaderFromGarbage) {
  // clang-format off
  const uint8_t kGarbage[] = {
    0x4f, 0x27, 0xeb, 0x22, 0x27, 0xeb, 0x22, 0x4f,
    0xeb, 0x22, 0x4f, 0x27, 0x22, 0x4f, 0x27, 0xeb,
  };
  // clang-format on
  EXPECT_FALSE(RtcpCommonHeader::Parse(kGarbage).has_value());
}

// Tests whether RTCP Common Header validation logic is correct.
TEST(RtcpCommonTest, WillNotParseHeaderWithInvalidData) {
  // clang-format off
  const uint8_t kCastFeedbackPacket[] = {
    0b10000001,  // Version=2, Padding=no, ItemCount=1 byte.
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

  // Start with a valid packet, and expect the parse to succeed.
  uint8_t buffer[sizeof(kCastFeedbackPacket)];
  memcpy(buffer, kCastFeedbackPacket, sizeof(buffer));
  EXPECT_TRUE(RtcpCommonHeader::Parse(buffer).has_value());

  // Wrong version in first byte: Expect parse failure.
  buffer[0] = 0b01000001;
  EXPECT_FALSE(RtcpCommonHeader::Parse(buffer).has_value());
  buffer[0] = kCastFeedbackPacket[0];

  // Wrong packet type (not in RTCP range): Expect parse failure.
  buffer[1] = 42;
  EXPECT_FALSE(RtcpCommonHeader::Parse(buffer).has_value());
  buffer[1] = kCastFeedbackPacket[1];
}

// Test that the Report Block optionally included in Sender Reports or Receiver
// Reports can be serialized and re-parsed correctly.
TEST(RtcpCommonTest, SerializesAndParsesRtcpReportBlocks) {
  constexpr Ssrc kSsrc{0x04050607};

  RtcpReportBlock original;
  original.ssrc = kSsrc;
  original.packet_fraction_lost_numerator = 0x67;
  original.cumulative_packets_lost = 74536;
  original.extended_high_sequence_number = 0x0201fedc;
  original.jitter = RtpTimeDelta::FromTicks(123);
  original.last_status_report_id = 0x0908;
  original.delay_since_last_report = RtcpReportBlock::Delay(99999);

  uint8_t buffer[kRtcpReportBlockSize];
  original.Serialize(buffer);

  // If the number of report blocks is zero, or some other SSRC is specified,
  // ParseOne() should not return a result.
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 0, 0).has_value());
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 0, kSsrc).has_value());
  EXPECT_FALSE(RtcpReportBlock::ParseOne(buffer, 1, 0).has_value());

  // Expect that the report block is parsed correctly.
  const auto parsed = RtcpReportBlock::ParseOne(buffer, 1, kSsrc);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(original.ssrc, parsed->ssrc);
  EXPECT_EQ(original.packet_fraction_lost_numerator,
            parsed->packet_fraction_lost_numerator);
  EXPECT_EQ(original.cumulative_packets_lost, parsed->cumulative_packets_lost);
  EXPECT_EQ(original.extended_high_sequence_number,
            parsed->extended_high_sequence_number);
  EXPECT_EQ(original.jitter, parsed->jitter);
  EXPECT_EQ(original.last_status_report_id, parsed->last_status_report_id);
  EXPECT_EQ(original.delay_since_last_report, parsed->delay_since_last_report);
}

// Tests that the Report Block parser can, among multiple Report Blocks, find
// the one with a matching recipient SSRC.
TEST(RtcpCommonTest, ParsesOneReportBlockFromMultipleBlocks) {
  constexpr Ssrc kSsrc{0x04050607};
  constexpr int kNumBlocks = 5;

  RtcpReportBlock expected;
  expected.ssrc = kSsrc;
  expected.packet_fraction_lost_numerator = 0x67;
  expected.cumulative_packets_lost = 74536;
  expected.extended_high_sequence_number = 0x0201fedc;
  expected.jitter = RtpTimeDelta::FromTicks(123);
  expected.last_status_report_id = 0x0908;
  expected.delay_since_last_report = RtcpReportBlock::Delay(99999);

  // Generate multiple report blocks with different recipient SSRCs.
  uint8_t buffer[kRtcpReportBlockSize * kNumBlocks];
  for (int i = 0; i < kNumBlocks; ++i) {
    RtcpReportBlock another;
    another.ssrc = expected.ssrc + i - 2;
    another.packet_fraction_lost_numerator =
        expected.packet_fraction_lost_numerator + i - 2;
    another.cumulative_packets_lost = expected.cumulative_packets_lost + i - 2;
    another.extended_high_sequence_number =
        expected.extended_high_sequence_number + i - 2;
    another.jitter = expected.jitter + RtpTimeDelta::FromTicks(i - 2);
    another.last_status_report_id = expected.last_status_report_id + i - 2;
    another.delay_since_last_report =
        expected.delay_since_last_report + RtcpReportBlock::Delay(i - 2);

    another.Serialize(absl::Span<uint8_t>(buffer + i * kRtcpReportBlockSize,
                                          kRtcpReportBlockSize));
  }

  // Expect that the desired report block is found and parsed correctly.
  const auto parsed = RtcpReportBlock::ParseOne(buffer, kNumBlocks, kSsrc);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(expected.ssrc, parsed->ssrc);
  EXPECT_EQ(expected.packet_fraction_lost_numerator,
            parsed->packet_fraction_lost_numerator);
  EXPECT_EQ(expected.cumulative_packets_lost, parsed->cumulative_packets_lost);
  EXPECT_EQ(expected.extended_high_sequence_number,
            parsed->extended_high_sequence_number);
  EXPECT_EQ(expected.jitter, parsed->jitter);
  EXPECT_EQ(expected.last_status_report_id, parsed->last_status_report_id);
  EXPECT_EQ(expected.delay_since_last_report, parsed->delay_since_last_report);
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
