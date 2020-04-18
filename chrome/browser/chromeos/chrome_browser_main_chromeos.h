// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_

#include <memory>

#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/chrome_browser_main_linux.h"
#include "chrome/browser/chromeos/external_metrics.h"
#include "chrome/browser/memory/memory_kills_monitor.h"
#include "chromeos/system/version_loader.h"

class NotificationPlatformBridge;
class SpokenFeedbackEventRewriterDelegate;

namespace lock_screen_apps {
class StateController;
}

namespace arc {
class ArcServiceLauncher;
class VoiceInteractionControllerClient;
}

namespace chromeos {

class ArcKioskAppManager;
class EventRewriterDelegateImpl;
class IdleActionWarningObserver;
class LowDiskNotification;
class NetworkPrefStateObserver;
class NetworkThrottlingObserver;
class PowerMetricsReporter;
class RendererFreezer;
class ShutdownPolicyForwarder;
class WakeOnWifiManager;

namespace default_app_order {
class ExternalLoader;
}


namespace internal {
class DBusPreEarlyInit;
class DBusServices;
class SystemTokenCertDBInitializer;
}

namespace power {
namespace ml {
class AdaptiveScreenBrightnessManager;
class UserActivityController;
}  // namespace ml
}  // namespace power

// ChromeBrowserMainParts implementation for chromeos specific code.
// NOTE: Chromeos UI (Ash) support should be added to
// ChromeBrowserMainExtraPartsAsh instead. This class should not depend on
// src/ash or chrome/browser/ui/ash.
class ChromeBrowserMainPartsChromeos : public ChromeBrowserMainPartsLinux {
 public:
  explicit ChromeBrowserMainPartsChromeos(
      const content::MainFunctionParams& parameters,
      std::unique_ptr<ui::DataPack> data_pack);
  ~ChromeBrowserMainPartsChromeos() override;

  // ChromeBrowserMainParts overrides.
  int PreEarlyInitialization() override;
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;

  // Stages called from PreMainMessageLoopRun.
  void PreProfileInit() override;
  void PostProfileInit() override;
  void PreBrowserStart() override;
  void PostBrowserStart() override;

  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;

 private:
  std::unique_ptr<default_app_order::ExternalLoader> app_order_loader_;
  std::unique_ptr<NetworkPrefStateObserver> network_pref_state_observer_;
  std::unique_ptr<IdleActionWarningObserver> idle_action_warning_observer_;
  std::unique_ptr<RendererFreezer> renderer_freezer_;
  std::unique_ptr<PowerMetricsReporter> power_metrics_reporter_;
  std::unique_ptr<WakeOnWifiManager> wake_on_wifi_manager_;
  std::unique_ptr<NetworkThrottlingObserver> network_throttling_observer_;

  std::unique_ptr<internal::DBusPreEarlyInit> dbus_pre_early_init_;
  std::unique_ptr<internal::DBusServices> dbus_services_;

  std::unique_ptr<internal::SystemTokenCertDBInitializer>
      system_token_certdb_initializer_;

  std::unique_ptr<ShutdownPolicyForwarder> shutdown_policy_forwarder_;

  std::unique_ptr<EventRewriterDelegateImpl> event_rewriter_delegate_;

  // Handles event dispatch to the spoken feedback extension (ChromeVox).
  std::unique_ptr<SpokenFeedbackEventRewriterDelegate>
      spoken_feedback_event_rewriter_delegate_;

  scoped_refptr<chromeos::ExternalMetrics> external_metrics_;

  std::unique_ptr<arc::ArcServiceLauncher> arc_service_launcher_;

  std::unique_ptr<arc::VoiceInteractionControllerClient>
      arc_voice_interaction_controller_client_;

  std::unique_ptr<LowDiskNotification> low_disk_notification_;
  std::unique_ptr<ArcKioskAppManager> arc_kiosk_app_manager_;

  std::unique_ptr<memory::MemoryKillsMonitor::Handle> memory_kills_monitor_;

  std::unique_ptr<lock_screen_apps::StateController>
      lock_screen_apps_state_controller_;

  // TODO(estade): Remove this when Chrome OS uses native notifications by
  // default (as it will be instantiated elsewhere). For now it's necessary to
  // send notifier settings information to Ash.
  std::unique_ptr<NotificationPlatformBridge> notification_client_;

  std::unique_ptr<power::ml::AdaptiveScreenBrightnessManager>
      adaptive_screen_brightness_manager_;

  std::unique_ptr<power::ml::UserActivityController> user_activity_controller_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainPartsChromeos);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_
