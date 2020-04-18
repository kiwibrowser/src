// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/devicemode.h"

#include "base/command_line.h"
#include "base/sys_info.h"
#include "chromeos/chromeos_switches.h"

namespace chromeos {

bool IsRunningAsSystemCompositor() {
  static bool is_running_on_chrome_os = base::SysInfo::IsRunningOnChromeOS();
  return is_running_on_chrome_os ||
         base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kForceSystemCompositorMode);
}

}  // namespace chromeos
