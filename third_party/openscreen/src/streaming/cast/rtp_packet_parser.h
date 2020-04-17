// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_RTP_PACKET_PARSER_H_
#define STREAMING_CAST_RTP_PACKET_PARSER_H_

#include <chrono>

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "streaming/cast/frame_id.h"
#include "streaming/cast/rtp_defines.h"
#include "streaming/cast/rtp_time.h"
#include "streaming/cast/ssrc.h"

namespace openscreen {
namespace cast_streaming {

// Parses RTP packets for all frames in the same Cast RTP stream. One
// RtpPacketParser instance should be used for all RTP packets having the same
// SSRC.
//
// Note that the parser is not stateless: One of its responsibilities is to
// bit-expand values that exist in a truncated form within the packets. It
// tracks the progression of those values in a live system to re-constitute such
// values.
class RtpPacketParser {
 public:
  struct ParseResult {
    // Elements from RTP packet header.
    // https://tools.ietf.org/html/rfc3550#section-5
    RtpPayloadType payload_type;
    uint16_t sequence_number;    // Wrap-around packet transmission counter.
    RtpTimeTicks rtp_timestamp;  // The media timestamp.

    // Elements from Cast header (at beginning of RTP payload).
    bool is_key_frame;
    FrameId frame_id;
    FramePacketId packet_id;  // Always in the range [0,max_packet_id].
    FramePacketId max_packet_id;
    FrameId referenced_frame_id;  // ID of frame required to decode this one.
    std::chrono::milliseconds new_playout_delay{};  // Ignore if non-positive.

    // Portion of the |packet| that was passed into Parse() that contains the
    // payload. WARNING: This memory region is only valid while the original
    // |packet| memory remains valid.
    absl::Span<const uint8_t> payload;

    ParseResult();
    ~ParseResult();
  };

  explicit RtpPacketParser(Ssrc sender_ssrc);
  ~RtpPacketParser();

  // Parses the packet. The caller should use InspectPacketForRouting()
  // beforehand to ensure that the packet is meant to be parsed by this
  // instance. Returns absl::nullopt if the |packet| was corrupt.
  absl::optional<ParseResult> Parse(absl::Span<const uint8_t> packet);

 private:
  const Ssrc sender_ssrc_;

  // Tracks recently-parsed RTP timestamps so that the truncated values can be
  // re-expanded into full-form.
  RtpTimeTicks last_parsed_rtp_timestamp_;

  // The highest frame ID seen in any RTP packets so far. This is tracked so
  // that the truncated frame ID fields in RTP packets can be re-expanded into
  // full-form.
  FrameId highest_rtp_frame_id_;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_RTP_PACKET_PARSER_H_
