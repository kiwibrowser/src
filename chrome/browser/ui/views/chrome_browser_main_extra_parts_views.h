// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CHROME_BROWSER_MAIN_EXTRA_PARTS_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_CHROME_BROWSER_MAIN_EXTRA_PARTS_VIEWS_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "ui/views/layout/layout_provider.h"

namespace views {
class ViewsDelegate;
}

#if defined(USE_AURA)
namespace ui_devtools {
class UiDevToolsServer;
}
namespace views {
class MusClient;
}
namespace wm {
class WMState;
}
#endif

#if !defined(OS_CHROMEOS)
class RelaunchNotificationController;
#endif

class ChromeBrowserMainExtraPartsViews : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsViews();
  ~ChromeBrowserMainExtraPartsViews() override;

  // Overridden from ChromeBrowserMainExtraParts:
  void ToolkitInitialized() override;
  void PreCreateThreads() override;
  void PreProfileInit() override;
  void ServiceManagerConnectionStarted(
      content::ServiceManagerConnection* connection) override;
  void PostBrowserStart() override;
  void PostMainMessageLoopRun() override;

 private:
  std::unique_ptr<views::ViewsDelegate> views_delegate_;
  std::unique_ptr<views::LayoutProvider> layout_provider_;

#if defined(USE_AURA)
  // Only used when running in --enable-ui-devtools.
  std::unique_ptr<ui_devtools::UiDevToolsServer> devtools_server_;

  // Not created when running in ash::Config::MUS.
  std::unique_ptr<wm::WMState> wm_state_;

  // Only used when running in ash::Config::MASH.
  std::unique_ptr<views::MusClient> mus_client_;
#endif

#if !defined(OS_CHROMEOS)
  // Manages the relaunch notification prompts.
  std::unique_ptr<RelaunchNotificationController>
      relaunch_notification_controller_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_CHROME_BROWSER_MAIN_EXTRA_PARTS_VIEWS_H_
