// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_WIN_H_
#define CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_WIN_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/browser_process_platform_part_base.h"

class DidRunUpdater;

namespace base {
class CommandLine;
}

class BrowserProcessPlatformPart : public BrowserProcessPlatformPartBase {
 public:
  BrowserProcessPlatformPart();
  ~BrowserProcessPlatformPart() override;

  // BrowserProcessPlatformPartBase:
  void PlatformSpecificCommandLineProcessing(
      const base::CommandLine& command_line) override;

 private:
  std::unique_ptr<DidRunUpdater> did_run_updater_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcessPlatformPart);
};

#endif  // CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_WIN_H_
