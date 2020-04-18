// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_process_util.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

MacChromeProcessInfoList GetRunningMacProcessInfo(
    const ChromeProcessList& process_list) {
  MacChromeProcessInfoList result;

  // Build up the ps command line
  std::vector<std::string> cmdline;
  cmdline.push_back("ps");
  cmdline.push_back("-o");
  cmdline.push_back("pid=,rss=,vsz=");  // fields we need, no headings
  ChromeProcessList::const_iterator process_iter;
  for (process_iter = process_list.begin();
       process_iter != process_list.end();
       ++process_iter) {
    cmdline.push_back("-p");
    cmdline.push_back(base::IntToString(*process_iter));
  }

  // Invoke it
  std::string ps_output;
  if (!base::GetAppOutput(base::CommandLine(cmdline), &ps_output))
    return result;  // All the pids might have exited

  // Process the results.
  for (const std::string& raw_line : base::SplitString(
           ps_output, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    std::string line(base::CollapseWhitespaceASCII(raw_line, false));
    std::vector<base::StringPiece> values = base::SplitStringPiece(
        line, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (values.size() == 3) {
      MacChromeProcessInfo proc_info;
      int pid;
      base::StringToInt(values[0], &pid);
      proc_info.pid = pid;
      base::StringToInt(values[1], &proc_info.rsz_in_kb);
      base::StringToInt(values[2], &proc_info.vsz_in_kb);
      if (proc_info.pid && proc_info.rsz_in_kb && proc_info.vsz_in_kb)
        result.push_back(proc_info);
    }
  }

  return result;
}
