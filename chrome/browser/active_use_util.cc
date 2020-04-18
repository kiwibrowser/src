// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/active_use_util.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"

#if defined(OS_WIN)
#include "chrome/install_static/install_modes.h"
#endif

bool ShouldRecordActiveUse(const base::CommandLine& command_line) {
#if defined(OS_WIN)
  if (!install_static::kUseGoogleUpdateIntegration)
    return false;
#endif
  return command_line.GetSwitchValueNative(switches::kTryChromeAgain).empty();
}
