// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtcp_common.h"

#include <limits>

#include "streaming/cast/packet_util.h"

namespace openscreen {
namespace cast_streaming {

RtcpCommonHeader::RtcpCommonHeader() = default;
RtcpCommonHeader::~RtcpCommonHeader() = default;

void RtcpCommonHeader::Serialize(absl::Span<uint8_t> buffer) const {
  OSP_CHECK_GE(buffer.size(), kRtcpCommonHeaderSize);

  uint8_t byte0 = kRtcpRequiredVersionAndPaddingBits
                  << kRtcpReportCountFieldNumBits;
  switch (packet_type) {
    case RtcpPacketType::kSenderReport:
    case RtcpPacketType::kReceiverReport:
      OSP_DCHECK_LE(with.report_count,
                    FieldBitmask<int>(kRtcpReportCountFieldNumBits));
      byte0 |= with.report_count;
      break;
    case RtcpPacketType::kApplicationDefined:
    case RtcpPacketType::kPayloadSpecific:
      switch (with.subtype) {
        case RtcpSubtype::kPictureLossIndicator:
        case RtcpSubtype::kFeedback:
          byte0 |= static_cast<uint8_t>(with.subtype);
          break;
        case RtcpSubtype::kReceiverLog:
          OSP_UNIMPLEMENTED();
          break;
        default:
          OSP_NOTREACHED();
          break;
      }
      break;
    case RtcpPacketType::kExtendedReports:
      break;
    case RtcpPacketType::kNull:
      OSP_NOTREACHED();
      break;
  }
  AppendField<uint8_t>(byte0, &buffer);

  AppendField<uint8_t>(static_cast<uint8_t>(packet_type), &buffer);

  // The size of the packet must be evenly divisible by the 32-bit word size.
  OSP_DCHECK_EQ(0, payload_size % sizeof(uint32_t));
  AppendField<uint16_t>(payload_size / sizeof(uint32_t), &buffer);
}

// static
absl::optional<RtcpCommonHeader> RtcpCommonHeader::Parse(
    absl::Span<const uint8_t> buffer) {
  if (buffer.size() < kRtcpCommonHeaderSize) {
    return absl::nullopt;
  }

  const uint8_t byte0 = ConsumeField<uint8_t>(&buffer);
  if ((byte0 >> kRtcpReportCountFieldNumBits) !=
      kRtcpRequiredVersionAndPaddingBits) {
    return absl::nullopt;
  }
  const uint8_t report_count_or_subtype =
      byte0 & FieldBitmask<uint8_t>(kRtcpReportCountFieldNumBits);

  const uint8_t byte1 = ConsumeField<uint8_t>(&buffer);
  if (!IsRtcpPacketType(byte1)) {
    return absl::nullopt;
  }

  // Optionally set |header.with.report_count| or |header.with.subtype|,
  // depending on the packet type.
  RtcpCommonHeader header;
  header.packet_type = static_cast<RtcpPacketType>(byte1);
  switch (header.packet_type) {
    case RtcpPacketType::kSenderReport:
    case RtcpPacketType::kReceiverReport:
      header.with.report_count = report_count_or_subtype;
      break;
    case RtcpPacketType::kApplicationDefined:
    case RtcpPacketType::kPayloadSpecific:
      switch (static_cast<RtcpSubtype>(report_count_or_subtype)) {
        case RtcpSubtype::kPictureLossIndicator:
        case RtcpSubtype::kReceiverLog:
        case RtcpSubtype::kFeedback:
          header.with.subtype =
              static_cast<RtcpSubtype>(report_count_or_subtype);
          break;
        default:  // Unknown subtype.
          header.with.subtype = RtcpSubtype::kNull;
          break;
      }
      break;
    default:
      // Neither |header.with.report_count| nor |header.with.subtype| are used.
      break;
  }

  header.payload_size =
      static_cast<int>(ConsumeField<uint16_t>(&buffer)) * sizeof(uint32_t);

  return header;
}

RtcpReportBlock::RtcpReportBlock() = default;
RtcpReportBlock::~RtcpReportBlock() = default;

void RtcpReportBlock::Serialize(absl::Span<uint8_t> buffer) const {
  OSP_CHECK_GE(buffer.size(), kRtcpReportBlockSize);

  AppendField<uint32_t>(ssrc, &buffer);
  OSP_DCHECK_GE(packet_fraction_lost_numerator,
                std::numeric_limits<uint8_t>::min());
  OSP_DCHECK_LE(packet_fraction_lost_numerator,
                std::numeric_limits<uint8_t>::max());
  OSP_DCHECK_GE(cumulative_packets_lost, 0);
  OSP_DCHECK_LE(cumulative_packets_lost,
                FieldBitmask<int>(kRtcpCumulativePacketsFieldNumBits));
  AppendField<uint32_t>(
      (static_cast<int>(packet_fraction_lost_numerator)
       << kRtcpCumulativePacketsFieldNumBits) |
          (static_cast<int>(cumulative_packets_lost) &
           FieldBitmask<uint32_t>(kRtcpCumulativePacketsFieldNumBits)),
      &buffer);
  AppendField<uint32_t>(extended_high_sequence_number, &buffer);
  const int64_t jitter_ticks = jitter / RtpTimeDelta::FromTicks(1);
  OSP_DCHECK_GE(jitter_ticks, 0);
  OSP_DCHECK_LE(jitter_ticks, int64_t{std::numeric_limits<uint32_t>::max()});
  AppendField<uint32_t>(jitter_ticks, &buffer);
  AppendField<uint32_t>(last_status_report_id, &buffer);
  const int64_t delay_ticks = delay_since_last_report.count();
  OSP_DCHECK_GE(delay_ticks, 0);
  OSP_DCHECK_LE(delay_ticks, int64_t{std::numeric_limits<uint32_t>::max()});
  AppendField<uint32_t>(delay_ticks, &buffer);
}

// static
absl::optional<RtcpReportBlock> RtcpReportBlock::ParseOne(
    absl::Span<const uint8_t> buffer,
    int report_count,
    Ssrc ssrc) {
  if (static_cast<int>(buffer.size()) < (kRtcpReportBlockSize * report_count)) {
    return absl::nullopt;
  }

  absl::optional<RtcpReportBlock> result;
  for (int block = 0; block < report_count; ++block) {
    if (ConsumeField<uint32_t>(&buffer) != ssrc) {
      // Skip-over report block meant for some other recipient.
      buffer.remove_prefix(kRtcpReportBlockSize - sizeof(uint32_t));
      continue;
    }

    RtcpReportBlock& report_block = result.emplace();
    report_block.ssrc = ssrc;
    const auto second_word = ConsumeField<uint32_t>(&buffer);
    report_block.packet_fraction_lost_numerator =
        second_word >> kRtcpCumulativePacketsFieldNumBits;
    report_block.cumulative_packets_lost =
        second_word &
        FieldBitmask<uint32_t>(kRtcpCumulativePacketsFieldNumBits);
    report_block.extended_high_sequence_number =
        ConsumeField<uint32_t>(&buffer);
    report_block.jitter =
        RtpTimeDelta::FromTicks(ConsumeField<uint32_t>(&buffer));
    report_block.last_status_report_id = ConsumeField<uint32_t>(&buffer);
    report_block.delay_since_last_report =
        RtcpReportBlock::Delay(ConsumeField<uint32_t>(&buffer));
  }
  return result;
}

RtcpSenderReport::RtcpSenderReport() = default;
RtcpSenderReport::~RtcpSenderReport() = default;

}  // namespace cast_streaming
}  // namespace openscreen
