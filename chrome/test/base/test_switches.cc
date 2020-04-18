// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/test_switches.h"
#include "build/build_config.h"
#include "ppapi/buildflags/buildflags.h"

namespace switches {

// Also emit full event trace logs for successful tests.
const char kAlsoEmitSuccessLogs[] = "also-emit-success-logs";

#if BUILDFLAG(ENABLE_PLUGINS)
// Makes browser pixel tests overwrite the reference if it does not match.
const char kRebaselinePixelTests[] = "rebaseline-pixel-tests";
#endif

}  // namespace switches
