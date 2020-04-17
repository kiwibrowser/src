// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "absl/types/span.h"
#include "streaming/cast/sender_report_builder.h"
#include "streaming/cast/sender_report_parser.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

constexpr Ssrc kSenderSsrc{1};
constexpr Ssrc kReceiverSsrc{2};

class SenderReportTest : public testing::Test {
 public:
  SenderReportBuilder* builder() { return &builder_; }
  SenderReportParser* parser() { return &parser_; }
  const NtpTimeConverter& ntp_converter() const {
    return session_.ntp_converter();
  }

 private:
  RtcpSession session_{kSenderSsrc, kReceiverSsrc};
  SenderReportBuilder builder_{&session_};
  SenderReportParser parser_{&session_};
};

// Tests that the compound RTCP packets containing a Sender Report alongside
// zero or more other messages can be parsed successfully.
TEST_F(SenderReportTest, Parsing) {
  // clang-format off
  const uint8_t kSenderReportPacket[] = {
    0b10000001,  // Version=2, Padding=no, ItemCount=1 byte.
    200,  // RTCP Packet type byte.
    0x00, 0x0c,  // Length of remainder of packet, in 32-bit words.
    0x00, 0x00, 0x00, 0x01,  // SSRC of sender.
    0xe0, 0x73, 0x2e, 0x54,  // NTP Timestamp (late evening on 2019-04-30).
        0x80, 0x00, 0x00, 0x00,
    0x00, 0x14, 0x99, 0x70,  // RTP Timestamp (15 seconds, 90kHz timebase).
    0x00, 0x00, 0x01, 0xff,  // Sender's Packet Count.
    0x00, 0x07, 0x11, 0x0d,  // Sender's Octet Count.
    0x00, 0x00, 0x00, 0x02,  // SSRC of receiver (to whom this report is for).
    0x00,  // Fraction lost.
    0x00, 0x00, 0x02,  // Cumulative Number of Packets Lost.
    0x00, 0x00, 0x38, 0x40,  // Highest Sequence Number Received.
    0x00, 0x00, 0x03, 0x84,  // Interarrival Jitter.
    0xaf, 0xd3, 0xff, 0x00,  // Sender Report ID.
    0x00, 0x00, 0x83, 0xfa,  // Delay since last Sender Report.
  };

  constexpr NtpTimestamp kNtpTimestampInSenderReport{0xe0732e5480000000};

  const uint8_t kOtherPacket[] = {
    0b10000000,  // Version=2, Padding=no, ItemCount=0 byte.
    204,  // RTCP Packet type byte.
    0x00, 0x01,  // Length of remainder of packet, in 32-bit words.
    0x00, 0x00, 0x00, 0x02,  // SSRC of receiver.
  };
  // clang-format on

  // A RTCP packet only containing non-sender-reports will not provide a Sender
  // Report result.
  EXPECT_FALSE(parser()->Parse(kOtherPacket));

  // A compound RTCP packet containing a Sender Report alongside other things
  // should be detected as "well-formed" by the parser and it should also
  // provide a Sender Report result. Also, it shouldn't matter what the ordering
  // is.
  const absl::Span<const uint8_t> kCompoundCombinations[2][2] = {
      {kSenderReportPacket, kOtherPacket},
      {kOtherPacket, kSenderReportPacket},
  };
  for (const auto& combo : kCompoundCombinations) {
    uint8_t compound_packet[sizeof(kSenderReportPacket) + sizeof(kOtherPacket)];
    memcpy(compound_packet, combo[0].data(), combo[0].size());
    memcpy(compound_packet + combo[0].size(), combo[1].data(), combo[1].size());

    const auto parsed = parser()->Parse(compound_packet);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(ntp_converter().ToLocalTime(kNtpTimestampInSenderReport),
              parsed->reference_time);
    EXPECT_EQ(RtpTimeTicks() + RtpTimeDelta::FromTicks(1350000),
              parsed->rtp_timestamp);
    EXPECT_EQ(uint32_t{0x1ff}, parsed->send_packet_count);
    EXPECT_EQ(uint32_t{0x7110d}, parsed->send_octet_count);
    ASSERT_TRUE(parsed->report_block.has_value());
    EXPECT_EQ(kReceiverSsrc, parsed->report_block->ssrc);
    // Note: RtcpReportBlock parsing is unit-tested elsewhere.
  }
}

// Tests that the SenderReportParser will not try to parse an empty packet.
TEST_F(SenderReportTest, WillNotParseEmptyPacket) {
  const uint8_t kEmptyPacket[] = {};
  EXPECT_FALSE(parser()->Parse(absl::Span<const uint8_t>(kEmptyPacket, 0)));
}

// Tests that the SenderReportParser will not parse anything from garbage data.
TEST_F(SenderReportTest, WillNotParseGarbage) {
  // clang-format off
  const uint8_t kGarbage[] = {
    0x4f, 0x27, 0xeb, 0x22, 0x27, 0xeb, 0x22, 0x4f,
    0xeb, 0x22, 0x4f, 0x27, 0x22, 0x4f, 0x27, 0xeb,
  };
  // clang-format on
  EXPECT_FALSE(parser()->Parse(kGarbage));
}

// Assuming that SenderReportTest.Parsing has been proven the implementation,
// this test checks that the builder produces RTCP packets that can be parsed.
TEST_F(SenderReportTest, BuildPackets) {
  for (int i = 0; i <= 1; ++i) {
    const bool with_report_block = (i == 1);

    RtcpSenderReport original;
    original.reference_time = platform::Clock::now();
    original.rtp_timestamp = RtpTimeTicks() + RtpTimeDelta::FromTicks(5);
    original.send_packet_count = 55;
    original.send_octet_count = 20044;
    if (with_report_block) {
      RtcpReportBlock& report_block = original.report_block.emplace();
      report_block.ssrc = kReceiverSsrc;
    }

    uint8_t buffer[kRtcpCommonHeaderSize + kRtcpSenderReportSize +
                   kRtcpReportBlockSize];
    memset(buffer, 0, sizeof(buffer));
    const auto result = builder()->BuildPacket(original, buffer);
    ASSERT_TRUE(result.first.data());
    const int expected_packet_size =
        sizeof(buffer) - (with_report_block ? 0 : kRtcpReportBlockSize);
    EXPECT_EQ(expected_packet_size, static_cast<int>(result.first.size()));
    EXPECT_EQ(ToStatusReportId(
                  ntp_converter().ToNtpTimestamp(original.reference_time)),
              result.second);

    const auto parsed = parser()->Parse(result.first);
    ASSERT_TRUE(parsed.has_value());
    // Note: The reference time can be off by one platform clock tick due to
    // a lossy conversion when going to and from the wire-format NtpTimestamps.
    // See the unit tests in ntp_time_unittest.cc for further discussion.
    EXPECT_LE(
        std::abs((original.reference_time - parsed->reference_time).count()),
        1);
    EXPECT_EQ(original.rtp_timestamp, parsed->rtp_timestamp);
    EXPECT_EQ(original.send_packet_count, parsed->send_packet_count);
    EXPECT_EQ(original.send_octet_count, parsed->send_octet_count);
    if (with_report_block) {
      ASSERT_TRUE(parsed->report_block.has_value());
      EXPECT_EQ(original.report_block->ssrc, parsed->report_block->ssrc);
      // Note: RtcpReportBlock serialization/parsing is unit-tested elsewhere.
    }
  }
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
