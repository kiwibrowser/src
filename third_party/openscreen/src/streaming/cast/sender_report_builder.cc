// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/sender_report_builder.h"

#include "platform/api/logging.h"
#include "streaming/cast/packet_util.h"

namespace openscreen {
namespace cast_streaming {

SenderReportBuilder::SenderReportBuilder(RtcpSession* session)
    : session_(session) {
  OSP_DCHECK(session_);
}

SenderReportBuilder::~SenderReportBuilder() = default;

std::pair<absl::Span<uint8_t>, StatusReportId> SenderReportBuilder::BuildPacket(
    const RtcpSenderReport& sender_report,
    absl::Span<uint8_t> buffer) const {
  OSP_CHECK_GE(buffer.size(), kRequiredBufferSize);

  uint8_t* const packet_begin = buffer.data();

  RtcpCommonHeader header;
  header.packet_type = RtcpPacketType::kSenderReport;
  header.payload_size = kRtcpSenderReportSize;
  if (sender_report.report_block) {
    header.with.report_count = 1;
    header.payload_size += kRtcpReportBlockSize;
  } else {
    header.with.report_count = 0;
  }
  header.Serialize(buffer);
  buffer.remove_prefix(kRtcpCommonHeaderSize);

  AppendField<uint32_t>(session_->sender_ssrc(), &buffer);
  const NtpTimestamp ntp_timestamp =
      session_->ntp_converter().ToNtpTimestamp(sender_report.reference_time);
  AppendField<uint64_t>(ntp_timestamp, &buffer);
  AppendField<uint32_t>(sender_report.rtp_timestamp.lower_32_bits(), &buffer);
  AppendField<uint32_t>(sender_report.send_packet_count, &buffer);
  AppendField<uint32_t>(sender_report.send_octet_count, &buffer);
  if (sender_report.report_block) {
    sender_report.report_block->Serialize(buffer);
    buffer.remove_prefix(kRtcpReportBlockSize);
  }

  uint8_t* const packet_end = buffer.data();
  return std::make_pair(
      absl::Span<uint8_t>(packet_begin, packet_end - packet_begin),
      ToStatusReportId(ntp_timestamp));
}

}  // namespace cast_streaming
}  // namespace openscreen
