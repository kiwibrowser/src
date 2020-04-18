// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BROWSER_MAIN_PARTS_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"
#include "content/shell/browser/shell_browser_main_parts.h"
#include "ppapi/buildflags/buildflags.h"

namespace content {

class ShellPluginServiceFilter;

class LayoutTestBrowserMainParts : public ShellBrowserMainParts {
 public:
  explicit LayoutTestBrowserMainParts(const MainFunctionParams& parameters);
  ~LayoutTestBrowserMainParts() override;

 private:
  void InitializeBrowserContexts() override;
  void InitializeMessageLoopContext() override;

#if BUILDFLAG(ENABLE_PLUGINS)
  std::unique_ptr<ShellPluginServiceFilter> plugin_service_filter_;
#endif

  DISALLOW_COPY_AND_ASSIGN(LayoutTestBrowserMainParts);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BROWSER_MAIN_PARTS_H_
