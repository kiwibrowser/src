// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/packet_util.h"

#include "streaming/cast/rtcp_common.h"
#include "streaming/cast/rtp_defines.h"

namespace openscreen {
namespace cast_streaming {

std::pair<ApparentPacketType, Ssrc> InspectPacketForRouting(
    absl::Span<const uint8_t> packet) {
  // Check for RTP packets first, since they are more frequent.
  if (packet.size() >= kRtpPacketMinValidSize &&
      packet[0] == kRtpRequiredFirstByte &&
      IsRtpPayloadType(packet[1] & kRtpPayloadTypeMask)) {
    constexpr int kOffsetToSsrcField = 8;
    return std::make_pair(
        ApparentPacketType::RTP,
        Ssrc{ReadBigEndian<uint32_t>(packet.data() + kOffsetToSsrcField)});
  }

  // While RTCP packets are valid if they consist of just the RTCP Common
  // Header, all the RTCP packet types processed by this implementation will
  // also have a SSRC field immediately following the header. This is important
  // for routing the packet to the correct parser instance.
  constexpr int kRtcpPacketMinAcceptableSize =
      kRtcpCommonHeaderSize + sizeof(uint32_t);
  if (packet.size() >= kRtcpPacketMinAcceptableSize &&
      RtcpCommonHeader::Parse(packet).has_value()) {
    return std::make_pair(
        ApparentPacketType::RTCP,
        Ssrc{ReadBigEndian<uint32_t>(packet.data() + kRtcpCommonHeaderSize)});
  }

  return std::make_pair(ApparentPacketType::UNKNOWN, Ssrc{0});
}

}  // namespace cast_streaming
}  // namespace openscreen
