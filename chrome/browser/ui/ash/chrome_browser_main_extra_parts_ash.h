// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_
#define CHROME_BROWSER_UI_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/common/buildflags.h"
#include "chromeos/assistant/buildflags.h"

namespace aura {
class UserActivityForwarder;
}

namespace chromeos {
class NetworkPortalNotificationController;
}

namespace ui {
class UserActivityDetector;
}

class AccessibilityControllerClient;
class AppListClientImpl;
class AshShellInit;
class AutoConnectNotifier;
class CastConfigClientMediaRouter;
class ChromeNewWindowClient;
class ChromeShellContentState;
class DataPromoNotification;
class ImeControllerClient;
class ImmersiveContextMus;
class ImmersiveHandlerFactoryMus;
class LoginScreenClient;
class MediaClient;
class NetworkConnectDelegateChromeOS;
class NightLightClient;
class SessionControllerClient;
class SystemTrayClient;
class TabletModeClient;
class VolumeController;
class VpnListForwarder;
class WallpaperControllerClient;

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
class ExoParts;
#endif

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
class AssistantClient;
#endif

namespace internal {
class ChromeLauncherControllerInitializer;
}

// Browser initialization for Ash UI. Only use this for Ash specific
// intitialization (e.g. initialization of chrome/browser/ui/ash classes).
class ChromeBrowserMainExtraPartsAsh : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsAsh();
  ~ChromeBrowserMainExtraPartsAsh() override;

  // Overridden from ChromeBrowserMainExtraParts:
  void ServiceManagerConnectionStarted(
      content::ServiceManagerConnection* connection) override;
  void PreProfileInit() override;
  void PostProfileInit() override;
  void PostBrowserStart() override;
  void PostMainMessageLoopRun() override;

 private:
  // Initialized in PreProfileInit in all configs before Shell init:
  std::unique_ptr<NetworkConnectDelegateChromeOS> network_connect_delegate_;

  // Initialized in PreProfileInit if ash config != MASH:
  std::unique_ptr<AshShellInit> ash_shell_init_;

  // Initialized in PreProfileInit if ash config == MASH:
  std::unique_ptr<ImmersiveContextMus> immersive_context_;
  std::unique_ptr<ImmersiveHandlerFactoryMus> immersive_handler_factory_;
  std::unique_ptr<aura::UserActivityForwarder> user_activity_forwarder_;
  std::unique_ptr<ui::UserActivityDetector> user_activity_detector_;

  // Initialized in PreProfileInit in all configs after Shell init:
  std::unique_ptr<AccessibilityControllerClient>
      accessibility_controller_client_;
  std::unique_ptr<AppListClientImpl> app_list_client_;
  std::unique_ptr<ChromeNewWindowClient> chrome_new_window_client_;
  std::unique_ptr<ImeControllerClient> ime_controller_client_;
  std::unique_ptr<SessionControllerClient> session_controller_client_;
  std::unique_ptr<SystemTrayClient> system_tray_client_;
  std::unique_ptr<TabletModeClient> tablet_mode_client_;
  std::unique_ptr<VolumeController> volume_controller_;
  std::unique_ptr<VpnListForwarder> vpn_list_forwarder_;
  std::unique_ptr<WallpaperControllerClient> wallpaper_controller_client_;
  // TODO(stevenjb): Move NetworkPortalNotificationController to c/b/ui/ash and
  // elim chromeos:: namespace. https://crbug.com/798569.
  std::unique_ptr<chromeos::NetworkPortalNotificationController>
      network_portal_notification_controller_;

  std::unique_ptr<internal::ChromeLauncherControllerInitializer>
      chrome_launcher_controller_initializer_;

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
  std::unique_ptr<ExoParts> exo_parts_;
#endif

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
  std::unique_ptr<AssistantClient> assistant_client_;
#endif

  // Initialized in PostProfileInit if ash config == MASH:
  std::unique_ptr<ChromeShellContentState> chrome_shell_content_state_;

  // Initialized in PostProfileInit in all configs:
  std::unique_ptr<AutoConnectNotifier> auto_connect_notifier_;
  std::unique_ptr<CastConfigClientMediaRouter> cast_config_client_media_router_;
  std::unique_ptr<LoginScreenClient> login_screen_client_;
  std::unique_ptr<MediaClient> media_client_;

  // Initialized in PostBrowserStart in all configs:
  std::unique_ptr<DataPromoNotification> data_promo_notification_;
  std::unique_ptr<NightLightClient> night_light_client_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsAsh);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_BROWSER_MAIN_EXTRA_PARTS_ASH_H_
