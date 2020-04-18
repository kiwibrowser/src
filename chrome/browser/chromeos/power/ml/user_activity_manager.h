// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_MANAGER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_ukm_logger.h"
#include "chrome/browser/resource_coordinator/tab_metrics_event.pb.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/policy.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/session_manager/core/session_manager.h"
#include "components/session_manager/core/session_manager_observer.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"
#include "ui/aura/window.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace chromeos {
namespace power {
namespace ml {

class BootClock;

struct TabProperty {
  // Whether the tab is the selected one in its containing browser.
  bool is_active;
  // Whether the containing browser is in focus.
  bool is_browser_focused;
  // Whether the containing browser is visible.
  bool is_browser_visible;
  // Whether the containing browser is the topmost one on the screen.
  bool is_topmost_browser;
  // Tab URL's engagement score. -1 if engagement service is disabled.
  int engagement_score;
  // Tab content type.
  // TODO(michaelpg): Move definition into user_activity_event.proto.
  metrics::TabMetricsEvent::ContentType content_type;
  // Whether user has form entry, i.e. text input.
  bool has_form_entry;
};

// Logs user activity after an idle event is observed.
// TODO(renjieliu): Add power-related activity as well.
class UserActivityManager : public ui::UserActivityObserver,
                            public IdleEventNotifier::Observer,
                            public PowerManagerClient::Observer,
                            public viz::mojom::VideoDetectorObserver,
                            public session_manager::SessionManagerObserver {
 public:
  UserActivityManager(UserActivityUkmLogger* ukm_logger,
                      IdleEventNotifier* idle_event_notifier,
                      ui::UserActivityDetector* detector,
                      chromeos::PowerManagerClient* power_manager_client,
                      session_manager::SessionManager* session_manager,
                      viz::mojom::VideoDetectorObserverRequest request,
                      const chromeos::ChromeUserManager* user_manager);
  ~UserActivityManager() override;

  // ui::UserActivityObserver overrides.
  void OnUserActivity(const ui::Event* event) override;

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void TabletModeEventReceived(chromeos::PowerManagerClient::TabletMode mode,
                               const base::TimeTicks& timestamp) override;
  void ScreenIdleStateChanged(
      const power_manager::ScreenIdleState& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void InactivityDelaysChanged(
      const power_manager::PowerManagementPolicy::Delays& delays) override;

  // viz::mojom::VideoDetectorObserver overrides:
  void OnVideoActivityStarted() override;
  void OnVideoActivityEnded() override {}

  // IdleEventNotifier::Observer overrides.
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& data) override;

  // session_manager::SessionManagerObserver overrides:
  void OnSessionStateChanged() override;

 private:
  friend class UserActivityManagerTest;

  // Updates lid state and tablet mode from received switch states.
  void OnReceiveSwitchStates(
      base::Optional<chromeos::PowerManagerClient::SwitchStates> switch_states);

  void OnReceiveInactivityDelays(
      base::Optional<power_manager::PowerManagementPolicy::Delays> delays);

  // Updates |open_tabs_| to hold current information about all open tabs.
  void UpdateOpenTabsURLs();

  // Extracts features from last known activity data and from device states.
  void ExtractFeatures(const IdleEventNotifier::ActivityData& activity_data);

  // Log event only when an idle event is observed.
  void MaybeLogEvent(UserActivityEvent::Event::Type type,
                     UserActivityEvent::Event::Reason reason);

  // Set the task runner for testing purpose.
  void SetTaskRunnerForTesting(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      std::unique_ptr<BootClock> test_boot_clock);

  // Open tab properties, indexed by the source IDs of their URLs.
  std::map<ukm::SourceId, TabProperty> open_tabs_;

  // Time when an idle event is received and we start logging. Null if an idle
  // event hasn't been observed.
  base::Optional<base::TimeDelta> idle_event_start_since_boot_;

  chromeos::PowerManagerClient::LidState lid_state_ =
      chromeos::PowerManagerClient::LidState::NOT_PRESENT;

  chromeos::PowerManagerClient::TabletMode tablet_mode_ =
      chromeos::PowerManagerClient::TabletMode::UNSUPPORTED;

  UserActivityEvent::Features::DeviceType device_type_ =
      UserActivityEvent::Features::UNKNOWN_DEVICE;

  base::Optional<power_manager::PowerSupplyProperties::ExternalPower>
      external_power_;

  // Battery percent. This is in the range [0.0, 100.0].
  base::Optional<float> battery_percent_;

  // Indicates whether the screen is locked.
  bool screen_is_locked_ = false;

  // Features extracted when receives an idle event.
  UserActivityEvent::Features features_;

  // It is RealBootClock, but will be set to FakeBootClock for tests.
  std::unique_ptr<BootClock> boot_clock_;

  UserActivityUkmLogger* const ukm_logger_;

  ScopedObserver<IdleEventNotifier, IdleEventNotifier::Observer>
      idle_event_observer_;
  ScopedObserver<ui::UserActivityDetector, ui::UserActivityObserver>
      user_activity_observer_;
  ScopedObserver<chromeos::PowerManagerClient,
                 chromeos::PowerManagerClient::Observer>
      power_manager_client_observer_;
  ScopedObserver<session_manager::SessionManager,
                 session_manager::SessionManagerObserver>
      session_manager_observer_;

  session_manager::SessionManager* const session_manager_;

  mojo::Binding<viz::mojom::VideoDetectorObserver> binding_;

  const chromeos::ChromeUserManager* const user_manager_;

  // Delays to dim and turn off the screen. Zero means disabled.
  base::TimeDelta screen_dim_delay_;
  base::TimeDelta screen_off_delay_;

  // Whether screen is currently dimmed/off.
  bool screen_dimmed_ = false;
  bool screen_off_ = false;
  // Whether screen dim/off occurred before final event was logged. They are
  // reset to false at the start of each idle event.
  bool screen_dim_occurred_ = false;
  bool screen_off_occurred_ = false;
  bool screen_lock_occurred_ = false;

  base::WeakPtrFactory<UserActivityManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityManager);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_MANAGER_H_
