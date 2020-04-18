// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/scoped_set_running_on_chromeos_for_testing.h"

#include "base/sys_info.h"

namespace chromeos {

ScopedSetRunningOnChromeOSForTesting::ScopedSetRunningOnChromeOSForTesting(
    const std::string& lsb_release,
    const base::Time& lsb_release_time) {
  base::SysInfo::SetChromeOSVersionInfoForTest(lsb_release, lsb_release_time);
}

ScopedSetRunningOnChromeOSForTesting::~ScopedSetRunningOnChromeOSForTesting() {
  base::SysInfo::SetChromeOSVersionInfoForTest("", base::Time());
}

}  // namespace chromeos
