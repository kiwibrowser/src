// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_packetizer.h"

#include "absl/types/optional.h"
#include "streaming/cast/frame_crypto.h"
#include "streaming/cast/rtp_defines.h"
#include "streaming/cast/rtp_packet_parser.h"
#include "streaming/cast/ssrc.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

constexpr RtpPayloadType kPayloadType = RtpPayloadType::kAudioOpus;

// Returns true if |needle| is fully within |haystack|.
bool IsSubspan(absl::Span<const uint8_t> needle,
               absl::Span<const uint8_t> haystack) {
  return (needle.data() >= haystack.data()) &&
         ((needle.data() + needle.size()) <=
          (haystack.data() + haystack.size()));
}

class RtpPacketizerTest : public testing::Test {
 public:
  RtpPacketizerTest() = default;
  ~RtpPacketizerTest() = default;

  RtpPacketizer* packetizer() { return &packetizer_; }

  EncryptedFrame CreateFrame(FrameId frame_id,
                             bool is_key_frame,
                             std::chrono::milliseconds new_playout_delay,
                             int payload_size) const {
    EncodedFrame frame;
    frame.dependency =
        is_key_frame ? EncodedFrame::KEY : EncodedFrame::DEPENDENT;
    frame.frame_id = frame_id;
    frame.referenced_frame_id = is_key_frame ? frame_id : (frame_id - 1);
    frame.rtp_timestamp = RtpTimeTicks() + RtpTimeDelta::FromTicks(987);
    frame.reference_time = platform::Clock::now();
    frame.new_playout_delay = new_playout_delay;

    frame.data.resize(payload_size);
    for (int i = 0; i < payload_size; ++i) {
      frame.data[i] = static_cast<uint8_t>(i);
    }

    return crypto_.Encrypt(frame);
  }

  // Generates one of the frame's packets, then parses it and checks for the
  // expected values. Thus, this test assumes PacketParser is already working
  // (i.e., all RtpPacketParser unit tests are passing).
  void TestGeneratePacket(const EncryptedFrame& frame,
                          FramePacketId packet_id) {
    SCOPED_TRACE(testing::Message() << "packet_id=" << packet_id);

    const int frame_payload_size = frame.data.size();
    constexpr int kExpectedRtpHeaderSize = 23;
    const int packet_payload_size =
        kMaxRtpPacketSizeForIpv4UdpOnEthernet - kExpectedRtpHeaderSize;
    const int final_packet_payload_size =
        frame_payload_size % packet_payload_size;
    const int num_packets = 1 + frame_payload_size / packet_payload_size;

    // Generate a RTP packet and parse it.
    uint8_t scratch[kMaxRtpPacketSizeForIpv4UdpOnEthernet];
    memset(scratch, 0, sizeof(scratch));
    const auto packet = packetizer_.GeneratePacket(frame, packet_id, scratch);
    ASSERT_TRUE(IsSubspan(packet, scratch));

    const auto result = parser_.Parse(packet);
    ASSERT_TRUE(result);

    // Check that RTP header fields match expected values.
    EXPECT_EQ(kPayloadType, result->payload_type);
    EXPECT_EQ(frame.rtp_timestamp, result->rtp_timestamp);
    EXPECT_EQ(frame.dependency == EncodedFrame::KEY, result->is_key_frame);
    EXPECT_EQ(frame.frame_id, result->frame_id);
    EXPECT_EQ(packet_id, result->packet_id);
    EXPECT_EQ(static_cast<FramePacketId>(num_packets - 1),
              result->max_packet_id);
    EXPECT_EQ(frame.referenced_frame_id, result->referenced_frame_id);

    // The sequence number field MUST be different for each packet, regardless
    // of whether the exact same packet is being re-generated.
    if (last_sequence_number_) {
      EXPECT_EQ(static_cast<uint16_t>(*last_sequence_number_ + 1),
                result->sequence_number);
    }
    last_sequence_number_ = result->sequence_number;

    // If there is a playout delay change starting with this |frame|, it must
    // only be mentioned in the first packet.
    if (packet_id == FramePacketId{0}) {
      EXPECT_EQ(frame.new_playout_delay, result->new_playout_delay);
    } else {
      EXPECT_EQ(std::chrono::milliseconds(0), result->new_playout_delay);
    }

    // Check that the RTP payload is correct for this packet.
    ASSERT_TRUE(IsSubspan(result->payload, packet));
    // Last packet is smaller, as its payload is just the remaining bytes.
    const int expected_payload_size = (int{packet_id} == (num_packets - 1))
                                          ? final_packet_payload_size
                                          : packet_payload_size;
    EXPECT_EQ(expected_payload_size, static_cast<int>(result->payload.size()));
    const absl::Span<const uint8_t> expected_bytes(
        frame.data.data() + (packet_id * packet_payload_size),
        expected_payload_size);
    EXPECT_EQ(expected_bytes, result->payload);
  }

 private:
  // The RtpPacketizer instance under test, plus some surrounding dependencies
  // to generate its input and examine its output.
  const Ssrc ssrc_{GenerateSsrc(true)};
  const FrameCrypto crypto_{FrameCrypto::GenerateRandomBytes(),
                            FrameCrypto::GenerateRandomBytes()};
  RtpPacketizer packetizer_{kPayloadType, ssrc_,
                            kMaxRtpPacketSizeForIpv4UdpOnEthernet};
  RtpPacketParser parser_{ssrc_};

  // absl::nullopt until the random starting sequence number, from the first
  // packet generated by TestGeneratePacket(), is known.
  absl::optional<uint16_t> last_sequence_number_;
};

// Tests that all packets are generated for one key frame, followed by 9 "delta"
// frames. The key frame is larger than the other frames, as is typical in a
// real-world usage scenario.
TEST_F(RtpPacketizerTest, GeneratesPacketsForSequenceOfFrames) {
  for (int i = 0; i < 10; ++i) {
    const bool is_key_frame = (i == 0);
    const int frame_payload_size = is_key_frame ? 48269 : 10000;
    const EncryptedFrame frame =
        CreateFrame(FrameId::first() + i, is_key_frame,
                    std::chrono::milliseconds(0), frame_payload_size);
    SCOPED_TRACE(testing::Message() << "frame_id=" << frame.frame_id);
    const int num_packets = packetizer()->ComputeNumberOfPackets(frame);
    ASSERT_EQ(is_key_frame ? 34 : 7, num_packets);

    for (int j = 0; j < num_packets; ++j) {
      TestGeneratePacket(frame, static_cast<FramePacketId>(j));
      if (testing::Test::HasFailure()) {
        return;
      }
    }
  }
}

// Tests that all packets are generated for a key frame that includes a playout
// delay change. Only the first packet should mention the playout delay change.
TEST_F(RtpPacketizerTest, GeneratesPacketsForFrameWithLatencyChange) {
  const int frame_payload_size = 38383;
  const EncryptedFrame frame =
      CreateFrame(FrameId::first() + 42, true, std::chrono::milliseconds(543),
                  frame_payload_size);
  const int num_packets = packetizer()->ComputeNumberOfPackets(frame);
  ASSERT_EQ(27, num_packets);

  for (int i = 0; i < num_packets; ++i) {
    TestGeneratePacket(frame, static_cast<FramePacketId>(i));
    if (testing::Test::HasFailure()) {
      return;
    }
  }
}

// Tests that a single, valid RTP packet is generated for a frame with no data
// payload. Having no payload is valid with some codecs (e.g., complete audio
// silence can be represented by an empty payload).
TEST_F(RtpPacketizerTest, GeneratesOnePacketForFrameWithNoPayload) {
  const int frame_payload_size = 0;
  const EncryptedFrame frame =
      CreateFrame(FrameId::first() + 99, false, std::chrono::milliseconds(0),
                  frame_payload_size);
  ASSERT_EQ(1, packetizer()->ComputeNumberOfPackets(frame));
  TestGeneratePacket(frame, FramePacketId{0});
}

// Tests that re-generating the same packet for re-transmission works, including
// a different sequence counter value in the packet each time.
TEST_F(RtpPacketizerTest, GeneratesPacketForRetransmission) {
  const int frame_payload_size = 16384;
  const EncryptedFrame frame = CreateFrame(
      FrameId::first(), true, std::chrono::milliseconds(0), frame_payload_size);
  const int num_packets = packetizer()->ComputeNumberOfPackets(frame);
  ASSERT_EQ(12, num_packets);

  for (int i = 0; i < 10; ++i) {
    // Keep generating the same packet. TestGeneratePacket() will check that a
    // different sequence number is used each time.
    TestGeneratePacket(frame, FramePacketId{3});
  }
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen
