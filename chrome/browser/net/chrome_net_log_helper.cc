// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_net_log_helper.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/common/chrome_switches.h"

net::NetLogCaptureMode GetNetCaptureModeFromCommandLine(
    const base::CommandLine& command_line) {
  if (command_line.HasSwitch(switches::kNetLogCaptureMode)) {
    std::string capture_mode_string =
        command_line.GetSwitchValueASCII(switches::kNetLogCaptureMode);
    if (capture_mode_string == "Default")
      return net::NetLogCaptureMode::Default();
    if (capture_mode_string == "IncludeCookiesAndCredentials")
      return net::NetLogCaptureMode::IncludeCookiesAndCredentials();
    if (capture_mode_string == "IncludeSocketBytes")
      return net::NetLogCaptureMode::IncludeSocketBytes();

    LOG(ERROR) << "Unrecognized value for --" << switches::kNetLogCaptureMode;
  }

  return net::NetLogCaptureMode::Default();
}
