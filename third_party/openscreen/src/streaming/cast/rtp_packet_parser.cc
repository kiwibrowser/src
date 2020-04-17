// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_packet_parser.h"

#include <algorithm>
#include <utility>

#include "platform/api/logging.h"
#include "streaming/cast/packet_util.h"

namespace openscreen {
namespace cast_streaming {

RtpPacketParser::RtpPacketParser(Ssrc sender_ssrc)
    : sender_ssrc_(sender_ssrc), highest_rtp_frame_id_(FrameId::first()) {}

RtpPacketParser::~RtpPacketParser() = default;

absl::optional<RtpPacketParser::ParseResult> RtpPacketParser::Parse(
    absl::Span<const uint8_t> buffer) {
  if (buffer.size() < kRtpPacketMinValidSize ||
      ConsumeField<uint8_t>(&buffer) != kRtpRequiredFirstByte) {
    return absl::nullopt;
  }

  // RTP header elements.
  //
  // Note: M (marker bit) is ignored here. Technically, according to the Cast
  // Streaming spec, it should only be set when PID == Max PID; but, let's be
  // lenient just in case some sender implementations don't adhere to this tiny,
  // subtle detail.
  const uint8_t payload_type =
      ConsumeField<uint8_t>(&buffer) & kRtpPayloadTypeMask;
  if (!IsRtpPayloadType(payload_type)) {
    return absl::nullopt;
  }
  ParseResult result;
  result.payload_type = static_cast<RtpPayloadType>(payload_type);
  result.sequence_number = ConsumeField<uint16_t>(&buffer);
  result.rtp_timestamp =
      last_parsed_rtp_timestamp_.Expand(ConsumeField<uint32_t>(&buffer));
  if (ConsumeField<uint32_t>(&buffer) != sender_ssrc_) {
    return absl::nullopt;
  }

  // Cast-specific header elements.
  const uint8_t byte12 = ConsumeField<uint8_t>(&buffer);
  result.is_key_frame = !!(byte12 & kRtpKeyFrameBitMask);
  const bool has_referenced_frame_id =
      !!(byte12 & kRtpHasReferenceFrameIdBitMask);
  const size_t num_cast_extensions = byte12 & kRtpExtensionCountMask;
  result.frame_id =
      highest_rtp_frame_id_.Expand(ConsumeField<uint8_t>(&buffer));
  result.packet_id = ConsumeField<uint16_t>(&buffer);
  result.max_packet_id = ConsumeField<uint16_t>(&buffer);
  if (result.packet_id > result.max_packet_id) {
    return absl::nullopt;
  }
  if (has_referenced_frame_id) {
    if (buffer.empty()) {
      return absl::nullopt;
    }
    result.referenced_frame_id =
        result.frame_id.Expand(ConsumeField<uint8_t>(&buffer));
  } else {
    // By default, if no reference frame ID was provided, the assumption is that
    // a key frame only references itself, while non-key frames reference only
    // their immediate predecessor.
    result.referenced_frame_id =
        result.is_key_frame ? result.frame_id : (result.frame_id - 1);
  }

  // Zero or more Cast extensions.
  for (size_t i = 0; i < num_cast_extensions; ++i) {
    if (buffer.size() < sizeof(uint16_t)) {
      return absl::nullopt;
    }
    const uint16_t type_and_size = ConsumeField<uint16_t>(&buffer);
    const uint8_t type = type_and_size >> kNumExtensionDataSizeFieldBits;
    const size_t size =
        type_and_size & FieldBitmask<uint16_t>(kNumExtensionDataSizeFieldBits);
    if (buffer.size() < size) {
      return absl::nullopt;
    }
    if (type == kAdaptiveLatencyRtpExtensionType) {
      if (size != sizeof(uint16_t)) {
        return absl::nullopt;
      }
      result.new_playout_delay =
          std::chrono::milliseconds(ReadBigEndian<uint16_t>(buffer.data()));
    }
    buffer.remove_prefix(size);
  }

  // All remaining data in the packet is the payload.
  result.payload = buffer;

  // At this point, the packet is known to be well-formed. Track recent field
  // values for later parses, to bit-extend the truncated values found in future
  // packets.
  last_parsed_rtp_timestamp_ = result.rtp_timestamp;
  highest_rtp_frame_id_ = std::max(highest_rtp_frame_id_, result.frame_id);

  return result;
}

RtpPacketParser::ParseResult::ParseResult() = default;
RtpPacketParser::ParseResult::~ParseResult() = default;

}  // namespace cast_streaming
}  // namespace openscreen
