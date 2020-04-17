// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_SENDER_REPORT_BUILDER_H_
#define STREAMING_CAST_SENDER_REPORT_BUILDER_H_

#include <stdint.h>

#include <utility>

#include "absl/types/span.h"
#include "streaming/cast/rtcp_common.h"
#include "streaming/cast/rtcp_session.h"
#include "streaming/cast/rtp_defines.h"

namespace openscreen {
namespace cast_streaming {

// Builds RTCP packets containing one Sender Report.
class SenderReportBuilder {
 public:
  explicit SenderReportBuilder(RtcpSession* session);
  ~SenderReportBuilder();

  // Serializes the given |sender_report| as a RTCP packet and writes it to
  // |buffer| (which must be kRequiredBufferSize in size). Returns the subspan
  // of |buffer| that contains the result and a StatusReportId the receiver
  // might use in its own reports to reference this specific report.
  std::pair<absl::Span<uint8_t>, StatusReportId> BuildPacket(
      const RtcpSenderReport& sender_report,
      absl::Span<uint8_t> buffer) const;

  // The required size (in bytes) of the buffer passed to BuildPacket().
  static constexpr int kRequiredBufferSize =
      kRtcpCommonHeaderSize + kRtcpSenderReportSize + kRtcpReportBlockSize;

 private:
  RtcpSession* const session_;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_SENDER_REPORT_BUILDER_H_
