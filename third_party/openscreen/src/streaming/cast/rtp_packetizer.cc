// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_packetizer.h"

#include <algorithm>
#include <limits>
#include <random>

#include "osp_base/big_endian.h"
#include "platform/api/logging.h"
#include "platform/api/time.h"
#include "streaming/cast/packet_util.h"

namespace openscreen {
namespace cast_streaming {

namespace {

// Returns a random sequence number to start with. The reason for using a random
// number instead of zero is unclear, but this has existed both in several
// versions of the Cast Streaming spec and in other implementations for many
// years.
uint16_t GenerateRandomSequenceNumberStart() {
  // Use a statically-allocated generator, instantiated upon first use, and
  // seeded with the current time tick count. This generator was chosen because
  // it is light-weight and does not need to produce unguessable (nor
  // crypto-secure) values.
  static std::minstd_rand generator(static_cast<std::minstd_rand::result_type>(
      platform::Clock::now().time_since_epoch().count()));

  return std::uniform_int_distribution<uint16_t>()(generator);
}

}  // namespace

RtpPacketizer::RtpPacketizer(RtpPayloadType payload_type,
                             Ssrc sender_ssrc,
                             int max_packet_size)
    : payload_type_7bits_(static_cast<uint8_t>(payload_type)),
      sender_ssrc_(sender_ssrc),
      max_packet_size_(max_packet_size),
      sequence_number_(GenerateRandomSequenceNumberStart()) {
  OSP_DCHECK(IsRtpPayloadType(payload_type_7bits_));
  OSP_DCHECK_GT(max_packet_size_, kMaxRtpHeaderSize);
}

RtpPacketizer::~RtpPacketizer() = default;

absl::Span<uint8_t> RtpPacketizer::GeneratePacket(const EncryptedFrame& frame,
                                                  FramePacketId packet_id,
                                                  absl::Span<uint8_t> buffer) {
  OSP_CHECK_GE(static_cast<int>(buffer.size()), max_packet_size_);

  const int num_packets = ComputeNumberOfPackets(frame);
  OSP_DCHECK_LT(int{packet_id}, num_packets);
  const bool is_last_packet = int{packet_id} == (num_packets - 1);

  // Compute the size of this packet, which is the number of bytes of header
  // plus the number of bytes of payload. Note that the optional Adaptive
  // Latency information is only added to the first packet.
  int packet_size = kBaseRtpHeaderSize;
  const bool include_adaptive_latency_change =
      (packet_id == 0 &&
       frame.new_playout_delay > std::chrono::milliseconds(0));
  if (include_adaptive_latency_change) {
    OSP_DCHECK_LE(frame.new_playout_delay.count(),
                  int{std::numeric_limits<uint16_t>::max()});
    packet_size += kAdaptiveLatencyHeaderSize;
  }
  int data_chunk_size = max_payload_size();
  const int data_chunk_start = data_chunk_size * int{packet_id};
  if (is_last_packet) {
    data_chunk_size = static_cast<int>(frame.data.size()) - data_chunk_start;
  }
  packet_size += data_chunk_size;
  OSP_DCHECK_LE(packet_size, max_packet_size_);
  const absl::Span<uint8_t> packet(buffer.data(), packet_size);

  // RTP Header.
  AppendField<uint8_t>(kRtpRequiredFirstByte, &buffer);
  AppendField<uint8_t>(
      (is_last_packet ? kRtpMarkerBitMask : 0) | payload_type_7bits_, &buffer);
  AppendField<uint16_t>(sequence_number_++, &buffer);
  AppendField<uint32_t>(frame.rtp_timestamp.lower_32_bits(), &buffer);
  AppendField<uint32_t>(sender_ssrc_, &buffer);

  // Cast Header.
  AppendField<uint8_t>(
      ((frame.dependency == EncodedFrame::KEY) ? kRtpKeyFrameBitMask : 0) |
          kRtpHasReferenceFrameIdBitMask |
          (include_adaptive_latency_change ? 1 : 0),
      &buffer);
  AppendField<uint8_t>(frame.frame_id.lower_8_bits(), &buffer);
  AppendField<uint16_t>(packet_id, &buffer);
  AppendField<uint16_t>(num_packets - 1, &buffer);
  AppendField<uint8_t>(frame.referenced_frame_id.lower_8_bits(), &buffer);

  // Extension of Cast Header for Adaptive Latency change.
  if (include_adaptive_latency_change) {
    AppendField<uint16_t>(
        (kAdaptiveLatencyRtpExtensionType << kNumExtensionDataSizeFieldBits) |
            sizeof(uint16_t),
        &buffer);
    AppendField<uint16_t>(frame.new_playout_delay.count(), &buffer);
  }

  // Sanity-check the pointer math, to ensure the packet is being entirely
  // populated, with no underrun or overrun.
  OSP_DCHECK_EQ(buffer.data() + data_chunk_size, packet.end());

  // Copy the encrypted payload data into the packet.
  memcpy(buffer.data(), frame.data.data() + data_chunk_start, data_chunk_size);

  return packet;
}

int RtpPacketizer::ComputeNumberOfPackets(const EncryptedFrame& frame) const {
  // The total number of packets is computed by assuming the payload will be
  // split-up across as few packets as possible.
  const auto DivideRoundingUp = [](int a, int b) { return (a + (b - 1)) / b; };
  int num_packets = DivideRoundingUp(frame.data.size(), max_payload_size());
  // Edge case: There must always be at least one packet, even when there are no
  // payload bytes. Some audio codecs, for example, use zero bytes to represent
  // a period of silence.
  num_packets = std::max(1, num_packets);

  // Ensure that the entire range of FramePacketIds can be represented.
  OSP_DCHECK_LE(num_packets, int{kMaxAllowedFramePacketId});
  return num_packets;
}

}  // namespace cast_streaming
}  // namespace openscreen
