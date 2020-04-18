// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_browser_main_extra_parts_ash.h"

#include <utility>

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/mus_property_mirror_ash.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/public/interfaces/process_creation_time_recorder.mojom.h"
#include "ash/shell.h"
#include "base/command_line.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/chromeos/night_light/night_light_client.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/app_list_client_impl.h"
#include "chrome/browser/ui/ash/accessibility/accessibility_controller_client.h"
#include "chrome/browser/ui/ash/ash_shell_init.h"
#include "chrome/browser/ui/ash/auto_connect_notifier.h"
#include "chrome/browser/ui/ash/cast_config_client_media_router.h"
#include "chrome/browser/ui/ash/chrome_new_window_client.h"
#include "chrome/browser/ui/ash/chrome_shell_content_state.h"
#include "chrome/browser/ui/ash/ime_controller_client.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chrome/browser/ui/ash/media_client.h"
#include "chrome/browser/ui/ash/network/data_promo_notification.h"
#include "chrome/browser/ui/ash/network/network_connect_delegate_chromeos.h"
#include "chrome/browser/ui/ash/network/network_portal_notification_controller.h"
#include "chrome/browser/ui/ash/session_controller_client.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/browser/ui/ash/tab_scrubber.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/ash/volume_controller.h"
#include "chrome/browser/ui/ash/vpn_list_forwarder.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/browser/ui/views/frame/immersive_context_mus.h"
#include "chrome/browser/ui/views/frame/immersive_handler_factory_mus.h"
#include "chrome/browser/ui/views/ime_driver/ime_driver_mus.h"
#include "chrome/browser/ui/views/select_file_dialog_extension.h"
#include "chrome/browser/ui/views/select_file_dialog_extension_factory.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/session_manager/core/session_manager.h"
#include "components/session_manager/core/session_manager_observer.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/user_activity_monitor.mojom.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/user_activity_forwarder.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/views/mus/mus_client.h"

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
#include "chrome/browser/exo_parts.h"
#endif

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
#include "chrome/browser/ui/ash/assistant/assistant_client.h"
#endif

namespace {

void PushProcessCreationTimeToAsh() {
  ash::mojom::ProcessCreationTimeRecorderPtr recorder;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &recorder);
  DCHECK(!startup_metric_utils::MainEntryPointTicks().is_null());
  recorder->SetMainProcessCreationTime(
      startup_metric_utils::MainEntryPointTicks());
}

}  // namespace

namespace internal {

// Creates a ChromeLauncherController on the first active session notification.
// Used to avoid constructing a ChromeLauncherController with no active profile.
class ChromeLauncherControllerInitializer
    : public session_manager::SessionManagerObserver {
 public:
  ChromeLauncherControllerInitializer() {
    session_manager::SessionManager::Get()->AddObserver(this);
  }

  ~ChromeLauncherControllerInitializer() override {
    if (!chrome_launcher_controller_)
      session_manager::SessionManager::Get()->RemoveObserver(this);
  }

  // session_manager::SessionManagerObserver:
  void OnSessionStateChanged() override {
    DCHECK(!chrome_launcher_controller_);
    DCHECK(!ChromeLauncherController::instance());

    if (session_manager::SessionManager::Get()->session_state() ==
        session_manager::SessionState::ACTIVE) {
      // Chrome keeps its own ShelfModel copy in sync with Ash's ShelfModel.
      chrome_shelf_model_ = std::make_unique<ash::ShelfModel>();
      chrome_launcher_controller_ = std::make_unique<ChromeLauncherController>(
          nullptr, chrome_shelf_model_.get());
      chrome_launcher_controller_->Init();

      session_manager::SessionManager::Get()->RemoveObserver(this);
    }
  }

 private:
  // By default |chrome_shelf_model_| is synced with Ash's ShelfController
  // instance in Mash and in Classic Ash; otherwise this is not created and
  // Ash's ShelfModel instance is used directly.
  std::unique_ptr<ash::ShelfModel> chrome_shelf_model_;
  std::unique_ptr<ChromeLauncherController> chrome_launcher_controller_;

  DISALLOW_COPY_AND_ASSIGN(ChromeLauncherControllerInitializer);
};

}  // namespace internal

ChromeBrowserMainExtraPartsAsh::ChromeBrowserMainExtraPartsAsh() {}

ChromeBrowserMainExtraPartsAsh::~ChromeBrowserMainExtraPartsAsh() {}

void ChromeBrowserMainExtraPartsAsh::ServiceManagerConnectionStarted(
    content::ServiceManagerConnection* connection) {
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    // ash::Shell will not be created because ash is running out-of-process.
    ash::Shell::SetIsBrowserProcessWithMash();
    DCHECK(views::MusClient::Exists());
    views::MusClient* mus_client = views::MusClient::Get();
    aura::WindowTreeClientDelegate* delegate = mus_client;
    // Register ash-specific window properties with Chrome's property converter.
    // Values of registered properties will be transported between the services.
    ash::RegisterWindowProperties(delegate->GetPropertyConverter());
    mus_client->SetMusPropertyMirror(
        std::make_unique<ash::MusPropertyMirrorAsh>());
  }
}

void ChromeBrowserMainExtraPartsAsh::PreProfileInit() {
  // IME driver must be available at login screen, so initialize before profile.
  IMEDriver::Register();

  // NetworkConnect handles the network connection state machine for the UI.
  network_connect_delegate_ =
      std::make_unique<NetworkConnectDelegateChromeOS>();
  chromeos::NetworkConnect::Initialize(network_connect_delegate_.get());

  if (chromeos::GetAshConfig() != ash::Config::MASH) {
    ash_shell_init_ = std::make_unique<AshShellInit>();
  } else {
    immersive_context_ = std::make_unique<ImmersiveContextMus>();
    immersive_handler_factory_ = std::make_unique<ImmersiveHandlerFactoryMus>();

    // Enterprise support in the browser can monitor user activity. Connect to
    // the UI service to monitor activity. The ash process has its own monitor.
    user_activity_detector_ = std::make_unique<ui::UserActivityDetector>();
    ui::mojom::UserActivityMonitorPtr user_activity_monitor;
    content::ServiceManagerConnection::GetForProcess()
        ->GetConnector()
        ->BindInterface(ui::mojom::kServiceName, &user_activity_monitor);
    user_activity_forwarder_ = std::make_unique<aura::UserActivityForwarder>(
        std::move(user_activity_monitor), user_activity_detector_.get());
  }

  app_list_client_ = std::make_unique<AppListClientImpl>();

  // Must be available at login screen, so initialize before profile.
  accessibility_controller_client_ =
      std::make_unique<AccessibilityControllerClient>();
  accessibility_controller_client_->Init();

  chrome_new_window_client_ = std::make_unique<ChromeNewWindowClient>();

  ime_controller_client_ = std::make_unique<ImeControllerClient>(
      chromeos::input_method::InputMethodManager::Get());
  ime_controller_client_->Init();

  session_controller_client_ = std::make_unique<SessionControllerClient>();
  session_controller_client_->Init();

  system_tray_client_ = std::make_unique<SystemTrayClient>();

  // Makes mojo request to TabletModeController in ash.
  tablet_mode_client_ = std::make_unique<TabletModeClient>();
  tablet_mode_client_->Init();

  volume_controller_ = std::make_unique<VolumeController>();

  vpn_list_forwarder_ = std::make_unique<VpnListForwarder>();

  wallpaper_controller_client_ = std::make_unique<WallpaperControllerClient>();
  wallpaper_controller_client_->Init();

  chrome_launcher_controller_initializer_ =
      std::make_unique<internal::ChromeLauncherControllerInitializer>();

  ui::SelectFileDialog::SetFactory(new SelectFileDialogExtensionFactory);

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
  exo_parts_ = ExoParts::CreateIfNecessary();
#endif

  PushProcessCreationTimeToAsh();
}

void ChromeBrowserMainExtraPartsAsh::PostProfileInit() {
  if (chromeos::GetAshConfig() == ash::Config::MASH)
    chrome_shell_content_state_ = std::make_unique<ChromeShellContentState>();

  cast_config_client_media_router_ =
      std::make_unique<CastConfigClientMediaRouter>();
  login_screen_client_ = std::make_unique<LoginScreenClient>();
  media_client_ = std::make_unique<MediaClient>();

  // Do not create a NetworkPortalNotificationController for tests since the
  // NetworkPortalDetector instance may be replaced.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kTestType)) {
    chromeos::NetworkPortalDetector* detector =
        chromeos::network_portal_detector::GetInstance();
    CHECK(detector);
    network_portal_notification_controller_ =
        std::make_unique<chromeos::NetworkPortalNotificationController>(
            detector);
  }

  // TODO(mash): Port TabScrubber.
  if (chromeos::GetAshConfig() != ash::Config::MASH) {
    // Initialize TabScrubber after the Ash Shell has been initialized.
    TabScrubber::GetInstance();
  }

  if (chromeos::NetworkHandler::IsInitialized() &&
      chromeos::NetworkHandler::Get()->auto_connect_handler()) {
    auto_connect_notifier_ = std::make_unique<AutoConnectNotifier>(
        ProfileManager::GetActiveUserProfile(),
        chromeos::NetworkHandler::Get()->network_connection_handler(),
        chromeos::NetworkHandler::Get()->network_state_handler(),
        chromeos::NetworkHandler::Get()->auto_connect_handler());
  }

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
  assistant_client_ = std::make_unique<AssistantClient>();
#endif
}

void ChromeBrowserMainExtraPartsAsh::PostBrowserStart() {
  data_promo_notification_ = std::make_unique<DataPromoNotification>();

  if (ash::switches::IsNightLightEnabled()) {
    night_light_client_ = std::make_unique<NightLightClient>(
        g_browser_process->system_request_context());
    night_light_client_->Start();
  }
}

void ChromeBrowserMainExtraPartsAsh::PostMainMessageLoopRun() {
#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
  // ExoParts uses state from ash, delete it before ash so that exo can
  // uninstall correctly.
  exo_parts_.reset();
#endif

  night_light_client_.reset();
  data_promo_notification_.reset();
#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
  assistant_client_.reset();
#endif
  chrome_launcher_controller_initializer_.reset();

  wallpaper_controller_client_.reset();
  vpn_list_forwarder_.reset();
  volume_controller_.reset();

  system_tray_client_.reset();
  session_controller_client_.reset();
  chrome_new_window_client_.reset();
  network_portal_notification_controller_.reset();
  media_client_.reset();
  login_screen_client_.reset();
  ime_controller_client_.reset();
  auto_connect_notifier_.reset();
  cast_config_client_media_router_.reset();
  accessibility_controller_client_.reset();

  // AppListClientImpl indirectly holds WebContents for answer card and
  // needs to be released before destroying the profile.
  app_list_client_.reset();

  ash_shell_init_.reset();

  chromeos::NetworkConnect::Shutdown();
  network_connect_delegate_.reset();

  // Views code observes TabletModeClient and may not be destroyed until
  // ash::Shell is so destroy |tablet_mode_client_| after ash::Shell.
  tablet_mode_client_.reset();
}
