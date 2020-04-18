// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "tools/accessibility/inspect/ax_tree_server.h"

char kPidSwitch[] = "pid";
char kWindowSwitch[] = "window";
char kFiltersSwitch[] = "filters";
char kJsonSwitch[] = "json";

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

  base::CommandLine::Init(argc, argv);

  base::string16 filters_path = base::ASCIIToUTF16(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kFiltersSwitch));

  bool use_json =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kJsonSwitch);

  std::string window_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kWindowSwitch);
  if (!window_str.empty()) {
    int window;
    if (StringToInt(window_str, &window)) {
      gfx::AcceleratedWidget widget(
          reinterpret_cast<gfx::AcceleratedWidget>(window));
      std::unique_ptr<content::AXTreeServer> server(
          new content::AXTreeServer(widget, filters_path, use_json));
      return 0;
    }
  }
  std::string pid_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kPidSwitch);
  if (!pid_str.empty()) {
    int pid;
    if (StringToInt(pid_str, &pid)) {
      base::ProcessId process_id = static_cast<base::ProcessId>(pid);
      std::unique_ptr<content::AXTreeServer> server(
          new content::AXTreeServer(process_id, filters_path, use_json));
    }
  }
  return 0;
}
