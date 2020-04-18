// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views.h"

#include <utility>

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/chrome_constrained_window_views_client.h"
#include "chrome/browser/ui/views/chrome_views_delegate.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/constrained_window/constrained_window_views.h"
#include "services/service_manager/sandbox/switches.h"

#if defined(USE_AURA)
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/chrome_switches.h"
#include "components/ui_devtools/css_agent.h"
#include "components/ui_devtools/devtools_server.h"
#include "components/ui_devtools/views/dom_agent_aura.h"
#include "components/ui_devtools/views/overlay_agent_aura.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/ui/public/cpp/gpu/gpu.h"  // nogncheck
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/aura/env.h"
#include "ui/display/screen.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/wm/core/wm_state.h"
#endif  // defined(USE_AURA)

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/common/content_switches.h"
#include "ui/base/l10n/l10n_util.h"
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS)

#if defined(OS_CHROMEOS)
#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "content/public/common/content_switches.h"
#else  // defined(OS_CHROMEOS)
#include "chrome/browser/ui/views/relaunch_notification/relaunch_notification_controller.h"
#endif  // defined(OS_CHROMEOS)

ChromeBrowserMainExtraPartsViews::ChromeBrowserMainExtraPartsViews() {
}

ChromeBrowserMainExtraPartsViews::~ChromeBrowserMainExtraPartsViews() {
  constrained_window::SetConstrainedWindowViewsClient(nullptr);
}

void ChromeBrowserMainExtraPartsViews::ToolkitInitialized() {
  // The delegate needs to be set before any UI is created so that windows
  // display the correct icon.
  if (!views::ViewsDelegate::GetInstance())
    views_delegate_ = std::make_unique<ChromeViewsDelegate>();

  SetConstrainedWindowViewsClient(CreateChromeConstrainedWindowViewsClient());

#if defined(USE_AURA)
  wm_state_.reset(new wm::WMState);
#endif
}

void ChromeBrowserMainExtraPartsViews::PreCreateThreads() {
#if defined(USE_AURA)
  views::InstallDesktopScreenIfNecessary();
#endif

  // TODO(pkasting): Try to move ViewsDelegate creation here as well;
  // see https://crbug.com/691894#c1
  // The layout_provider_ must be intialized here instead of in
  // ToolkitInitialized() because it relies on
  // ui::MaterialDesignController::Intialize() having already been called.
  if (!views::LayoutProvider::Get())
    layout_provider_ = ChromeLayoutProvider::CreateLayoutProvider();
}

void ChromeBrowserMainExtraPartsViews::PreProfileInit() {
#if defined(USE_AURA)
  // Start devtools server
  network::mojom::NetworkContext* network_context =
      g_browser_process->system_network_context_manager()->GetContext();
  devtools_server_ = ui_devtools::UiDevToolsServer::Create(
      network_context, switches::kEnableUiDevTools, 9223);
  if (devtools_server_) {
    auto dom_backend = std::make_unique<ui_devtools::DOMAgentAura>();
    auto overlay_backend =
        std::make_unique<ui_devtools::OverlayAgentAura>(dom_backend.get());
    auto css_backend =
        std::make_unique<ui_devtools::CSSAgent>(dom_backend.get());
    auto devtools_client = std::make_unique<ui_devtools::UiDevToolsClient>(
        "UiDevToolsClient", devtools_server_.get());
    devtools_client->AddAgent(std::move(dom_backend));
    devtools_client->AddAgent(std::move(css_backend));
    devtools_client->AddAgent(std::move(overlay_backend));
    devtools_server_->AttachClient(std::move(devtools_client));
  }
#endif

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // On the Linux desktop, we want to prevent the user from logging in as root,
  // so that we don't destroy the profile. Now that we have some minimal ui
  // initialized, check to see if we're running as root and bail if we are.
  if (geteuid() != 0)
    return;

  // Allow running inside an unprivileged user namespace. In that case, the
  // root directory will be owned by an unmapped UID and GID (although this
  // may not be the case if a chroot is also being used).
  struct stat st;
  if (stat("/", &st) == 0 && st.st_uid != 0)
    return;

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(service_manager::switches::kNoSandbox))
    return;

  base::string16 title = l10n_util::GetStringFUTF16(
      IDS_REFUSE_TO_RUN_AS_ROOT, l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  base::string16 message = l10n_util::GetStringFUTF16(
      IDS_REFUSE_TO_RUN_AS_ROOT_2, l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));

  chrome::ShowWarningMessageBox(NULL, title, message);

  // Avoids gpu_process_transport_factory.cc(153)] Check failed:
  // per_compositor_data_.empty() when quit is chosen.
  base::RunLoop().RunUntilIdle();

  exit(EXIT_FAILURE);
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS)
}

void ChromeBrowserMainExtraPartsViews::ServiceManagerConnectionStarted(
    content::ServiceManagerConnection* connection) {
  DCHECK(connection);
#if defined(USE_AURA)
  if (aura::Env::GetInstance()->mode() == aura::Env::Mode::LOCAL)
    return;

#if defined(OS_CHROMEOS)
  // Start up the window service and the ash system UI service.
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    connection->GetConnector()->StartService(
        service_manager::Identity(ui::mojom::kServiceName));
    connection->GetConnector()->StartService(
        service_manager::Identity(ash::mojom::kServiceName));
  }
#endif

#if defined(OS_CHROMEOS)
  if (chromeos::GetAshConfig() != ash::Config::MASH)
    return;
#endif

  views::MusClient::InitParams params;
  params.connector = connection->GetConnector();
  params.io_task_runner = content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::IO);
  // WMState is owned as a member, so don't have MusClient create it.
  params.create_wm_state = false;
  mus_client_ = std::make_unique<views::MusClient>(params);
#endif  // defined(USE_AURA)
}

void ChromeBrowserMainExtraPartsViews::PostBrowserStart() {
#if !defined(OS_CHROMEOS)
  relaunch_notification_controller_ =
      std::make_unique<RelaunchNotificationController>(
          UpgradeDetector::GetInstance());
#endif
}

void ChromeBrowserMainExtraPartsViews::PostMainMessageLoopRun() {
#if !defined(OS_CHROMEOS)
  // The relaunch notification controller acts on timer-based events. Tear it
  // down explicitly here to avoid a case where such an event arrives during
  // shutdown.
  relaunch_notification_controller_.reset();
#endif
}
