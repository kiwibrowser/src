// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "streaming/cast/sender_report_parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using openscreen::cast_streaming::RtcpSenderReport;
  using openscreen::cast_streaming::RtcpSession;
  using openscreen::cast_streaming::SenderReportParser;
  using openscreen::cast_streaming::Ssrc;

  constexpr Ssrc kSenderSsrcInSeedCorpus = 1;
  constexpr Ssrc kReceiverSsrcInSeedCorpus = 2;

  // Allocate the RtcpSession and SenderReportParser statically (i.e., one-time
  // init) to improve the fuzzer's execution rate. This is because RtcpSession
  // also contains a NtpTimeConverter, which samples the system clock at
  // construction time. There is no reason to re-construct these objects for
  // each fuzzer test input.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
  static RtcpSession session(kSenderSsrcInSeedCorpus,
                             kReceiverSsrcInSeedCorpus);
  static SenderReportParser parser(&session);
#pragma clang diagnostic pop

  parser.Parse(absl::Span<const uint8_t>(data, size));

  return 0;
}

#if defined(NEEDS_MAIN_TO_CALL_FUZZER_DRIVER)

// Forward declarations of Clang's built-in libFuzzer driver.
namespace fuzzer {
using TestOneInputCallback = int (*)(const uint8_t* data, size_t size);
int FuzzerDriver(int* argc, char*** argv, TestOneInputCallback callback);
}  // namespace fuzzer

int main(int argc, char* argv[]) {
  return fuzzer::FuzzerDriver(&argc, &argv, LLVMFuzzerTestOneInput);
}

#endif  // defined(NEEDS_MAIN_TO_CALL_FUZZER_DRIVER)
