// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_SENDER_REPORT_PARSER_H_
#define STREAMING_CAST_SENDER_REPORT_PARSER_H_

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "streaming/cast/rtcp_common.h"
#include "streaming/cast/rtcp_session.h"
#include "streaming/cast/rtp_defines.h"
#include "streaming/cast/rtp_time.h"

namespace openscreen {
namespace cast_streaming {

// Parses RTCP packets from a Sender to extract Sender Reports. Ignores anything
// else, since that is all a Receiver would be interested in.
class SenderReportParser {
 public:
  explicit SenderReportParser(RtcpSession* session);
  ~SenderReportParser();

  // Parses the RTCP |packet|, and returns a parsed sender report if the packet
  // contained one. Returns nullopt if the data is corrupt or the packet did not
  // contain a sender report.
  absl::optional<RtcpSenderReport> Parse(absl::Span<const uint8_t> packet);

 private:
  RtcpSession* const session_;

  // Tracks the recently-parsed RTP timestamps so that the truncated values can
  // be re-expanded into full-form.
  RtpTimeTicks last_parsed_rtp_timestamp_;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_SENDER_REPORT_PARSER_H_
