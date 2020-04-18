// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "tools/accessibility/inspect/ax_event_server.h"

char kPidSwitch[] = "pid";

// Convert from string to int, whether in 0x hex format or decimal format.
bool StringToInt(std::string str, int* result) {
  if (str.empty())
    return false;
  bool is_hex =
      str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
  return is_hex ? base::HexStringToInt(str, result)
                : base::StringToInt(str, result);
}

int main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;
  // TODO(aleventhal) Want callback after Ctrl+C or some global keystroke:
  // base::AtExitManager::RegisterCallback(content::OnExit, nullptr);

  base::CommandLine::Init(argc, argv);
  std::string pid_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kPidSwitch);
  int pid;
  if (!pid_str.empty()) {
    if (StringToInt(pid_str, &pid)) {
      std::unique_ptr<content::AXEventServer> server(
          new content::AXEventServer(pid));
    }
  }

  return 0;
}
