// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_defines.h"

namespace openscreen {
namespace cast_streaming {

bool IsRtpPayloadType(uint8_t raw_byte) {
  switch (static_cast<RtpPayloadType>(raw_byte)) {
    case RtpPayloadType::kAudioOpus:
    case RtpPayloadType::kAudioAac:
    case RtpPayloadType::kAudioPcm16:
    case RtpPayloadType::kAudioVarious:
    case RtpPayloadType::kVideoVp8:
    case RtpPayloadType::kVideoH264:
    case RtpPayloadType::kVideoVarious:
      return true;

    case RtpPayloadType::kNull:
      break;
  }
  return false;
}

bool IsRtcpPacketType(uint8_t raw_byte) {
  switch (static_cast<RtcpPacketType>(raw_byte)) {
    case RtcpPacketType::kSenderReport:
    case RtcpPacketType::kReceiverReport:
    case RtcpPacketType::kApplicationDefined:
    case RtcpPacketType::kPayloadSpecific:
    case RtcpPacketType::kExtendedReports:
      return true;

    case RtcpPacketType::kNull:
      break;
  }
  return false;
}

}  // namespace cast_streaming
}  // namespace openscreen
