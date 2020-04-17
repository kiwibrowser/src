// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "streaming/cast/rtp_packet_parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using openscreen::cast_streaming::RtpPacketParser;
  using openscreen::cast_streaming::Ssrc;

  constexpr Ssrc kSenderSsrcInSeedCorpus = 0x01020304;
  RtpPacketParser parser(kSenderSsrcInSeedCorpus);
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
