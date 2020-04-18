/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string>

#include "rtc_tools/rtp_generator/rtp_generator.h"
#include "rtc_tools/simple_command_line_parser.h"

int main(int argc, char* argv[]) {
  const std::string usage =
      "Generates custom configured rtpdumps for the purpose of testing.\n"
      "Example Usage:\n"
      "./rtp_generator --input_config=sender_config.json\n"
      "                --output_rtpdump=my.rtpdump\n";

  webrtc::test::CommandLineParser cmd_parser;
  cmd_parser.Init(argc, argv);
  cmd_parser.SetUsageMessage(usage);
  cmd_parser.SetFlag("input_config", "");
  cmd_parser.SetFlag("output_rtpdump", "");
  cmd_parser.ProcessFlags();

  const std::string config_path = cmd_parser.GetFlag("input_config");
  const std::string rtp_dump_path = cmd_parser.GetFlag("output_rtpdump");

  if (cmd_parser.GetFlag("help") == "true" || rtp_dump_path.empty() ||
      config_path.empty()) {
    cmd_parser.PrintUsageMessage();
    return EXIT_FAILURE;
  }

  absl::optional<webrtc::RtpGeneratorOptions> options =
      webrtc::ParseRtpGeneratorOptionsFromFile(config_path);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  webrtc::RtpGenerator rtp_generator(*options);
  rtp_generator.GenerateRtpDump(rtp_dump_path);

  return EXIT_SUCCESS;
}
