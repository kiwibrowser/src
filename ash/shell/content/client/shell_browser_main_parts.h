// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_CONTENT_CLIENT_SHELL_BROWSER_MAIN_PARTS_H_
#define ASH_SHELL_CONTENT_CLIENT_SHELL_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/browser_main_parts.h"

namespace content {
class ShellBrowserContext;
struct MainFunctionParams;
}

namespace net {
class NetLog;
}

namespace views {
class ViewsDelegate;
}

namespace wm {
class WMState;
}

namespace ash {
namespace shell {

class ExampleSessionControllerClient;
class WindowWatcher;

class ShellBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit ShellBrowserMainParts(const content::MainFunctionParams& parameters);
  ~ShellBrowserMainParts() override;

  // Overridden from content::BrowserMainParts:
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostMainMessageLoopRun() override;

  content::ShellBrowserContext* browser_context() {
    return browser_context_.get();
  }

 private:
  std::unique_ptr<net::NetLog> net_log_;
  std::unique_ptr<content::ShellBrowserContext> browser_context_;
  std::unique_ptr<views::ViewsDelegate> views_delegate_;
  std::unique_ptr<WindowWatcher> window_watcher_;
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<ExampleSessionControllerClient>
      example_session_controller_client_;

  DISALLOW_COPY_AND_ASSIGN(ShellBrowserMainParts);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_CONTENT_CLIENT_SHELL_BROWSER_MAIN_PARTS_H_
