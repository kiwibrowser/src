// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Wrapper to the parameter list for the "main" entry points (browser, renderer,
// plugin) to shield the call sites from the differences between platforms
// (e.g., POSIX doesn't need to pass any sandbox information).

#ifndef CONTENT_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_
#define CONTENT_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_

#include "base/callback_forward.h"
#include "base/command_line.h"
#include "build/build_config.h"

#if defined(OS_WIN)
namespace sandbox {
struct SandboxInterfaceInfo;
}
#elif defined(OS_MACOSX)
namespace base {
namespace mac {
class ScopedNSAutoreleasePool;
}
}
#endif

namespace content {

class BrowserMainParts;

using CreatedMainPartsClosure = base::Callback<void(BrowserMainParts*)>;

struct MainFunctionParams {
  explicit MainFunctionParams(const base::CommandLine& cl) : command_line(cl) {}

  const base::CommandLine& command_line;

#if defined(OS_WIN)
  sandbox::SandboxInterfaceInfo* sandbox_info = nullptr;
#elif defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool* autorelease_pool = nullptr;
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
  bool zygote_child = false;
#endif

  // TODO(sky): fix ownership of these tasks. MainFunctionParams should really
  // be passed as an r-value, at which point these can be unique_ptrs. For the
  // time ownership is passed with MainFunctionParams (meaning these are deleted
  // in content or client code).

  // Used by InProcessBrowserTest. If non-null BrowserMain schedules this
  // task to run on the MessageLoop and BrowserInit is not invoked.
  base::Closure* ui_task = nullptr;

  // Used by InProcessBrowserTest. If non-null this is Run() after
  // BrowserMainParts has been created and before PreEarlyInitialization().
  CreatedMainPartsClosure* created_main_parts_closure = nullptr;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_
