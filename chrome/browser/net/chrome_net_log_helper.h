// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_NET_LOG_HELPER_H_
#define CHROME_BROWSER_NET_CHROME_NET_LOG_HELPER_H_

#include "net/log/net_log_capture_mode.h"

namespace base {
class CommandLine;
}

// Returns the capture mode to be used with the net log.
net::NetLogCaptureMode GetNetCaptureModeFromCommandLine(
    const base::CommandLine& command_line);

#endif  // CHROME_BROWSER_NET_CHROME_NET_LOG_HELPER_H_
