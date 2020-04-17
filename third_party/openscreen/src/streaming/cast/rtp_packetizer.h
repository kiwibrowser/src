// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_RTP_PACKETIZER_H_
#define STREAMING_CAST_RTP_PACKETIZER_H_

#include <stdint.h>

#include "absl/types/span.h"
#include "streaming/cast/frame_crypto.h"
#include "streaming/cast/rtp_defines.h"
#include "streaming/cast/ssrc.h"

namespace openscreen {
namespace cast_streaming {

// Transforms a logical sequence of EncryptedFrames into RTP packets for
// transmission. A single instance of RtpPacketizer should be used for all the
// frames in a Cast RTP stream having the same SSRC.
class RtpPacketizer {
 public:
  // |payload_type| describes the type of the media content for the RTP stream
  // from the sender having the given |sender_ssrc|.
  //
  // The |max_packet_size| argument depends on the optimal over-the-wire size of
  // packets for the network medium being used. See discussion in rtp_defines.h
  // for further info.
  RtpPacketizer(RtpPayloadType payload_type,
                Ssrc sender_ssrc,
                int max_packet_size);

  ~RtpPacketizer();

  // Wire-format one of the RTP packets for the given frame, which must only be
  // transmitted once. This method should be called in the same sequence that
  // packets will be transmitted. This also means that, if a packet needs to be
  // re-transmitted, this method should be called to generate it again. Returns
  // the subspan of |buffer| that contains the packet. |buffer| must be at least
  // as large as the |max_packet_size| passed to the constructor.
  absl::Span<uint8_t> GeneratePacket(const EncryptedFrame& frame,
                                     FramePacketId packet_id,
                                     absl::Span<uint8_t> buffer);

  // Given |frame|, compute the total number of packets over which the whole
  // frame will be split-up.
  int ComputeNumberOfPackets(const EncryptedFrame& frame) const;

 private:
  int max_payload_size() const {
    // Start with the configured max packet size, then subtract reserved space
    // for packet header fields. The rest can be allocated to the payload.
    return max_packet_size_ - kMaxRtpHeaderSize;
  }

  // The validated ctor RtpPayloadType arg, in wire-format form.
  const uint8_t payload_type_7bits_;

  const Ssrc sender_ssrc_;
  const int max_packet_size_;

  // Incremented each time GeneratePacket() is called. Every packet, even those
  // re-transmitted, must have different sequence numbers (within wrap-around
  // concerns) per the RTP spec.
  uint16_t sequence_number_;

  // See rtp_defines.h for wire-format diagram.
  static constexpr int kBaseRtpHeaderSize =
      // Plus one byte, because this implementation always includes the 8-bit
      // Reference Frame ID field.
      kRtpPacketMinValidSize + 1;
  static constexpr int kAdaptiveLatencyHeaderSize = 4;
  static constexpr int kMaxRtpHeaderSize =
      kBaseRtpHeaderSize + kAdaptiveLatencyHeaderSize;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_RTP_PACKETIZER_H_
