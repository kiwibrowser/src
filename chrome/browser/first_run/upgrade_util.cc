// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/upgrade_util.h"

#include "base/command_line.h"
#include "base/logging.h"

namespace {

base::CommandLine* command_line;

}  // namespace

namespace upgrade_util {

void SetNewCommandLine(base::CommandLine* new_command_line) {
  command_line = new_command_line;
}

void RelaunchChromeBrowserWithNewCommandLineIfNeeded() {
  if (command_line) {
    if (!RelaunchChromeBrowser(*command_line)) {
      DLOG(ERROR) << "Launching a new instance of the browser failed.";
    } else {
      DLOG(WARNING) << "Launched a new instance of the browser.";
    }
    delete command_line;
    command_line = NULL;
  }
}

}  // namespace upgrade_util
