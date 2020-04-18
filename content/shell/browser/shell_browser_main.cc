// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_browser_main.h"

#include <iostream>
#include <memory>

#include "base/logging.h"
#include "build/build_config.h"
#include "content/public/browser/browser_main_runner.h"

#if defined(OS_ANDROID)
#include "base/run_loop.h"
#endif

// Main routine for running as the Browser process.
int ShellBrowserMain(
    const content::MainFunctionParams& parameters,
    const std::unique_ptr<content::BrowserMainRunner>& main_runner) {
  int exit_code = main_runner->Initialize(parameters);
  DCHECK_LT(exit_code, 0)
      << "BrowserMainRunner::Initialize failed in ShellBrowserMain";

  if (exit_code >= 0)
    return exit_code;

#if !defined(OS_ANDROID)
  exit_code = main_runner->Run();

  main_runner->Shutdown();
#endif

  return exit_code;
}
