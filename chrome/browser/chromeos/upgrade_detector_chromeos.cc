// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/upgrade_detector_chromeos.h"

#include <stdint.h>

#include <utility>

#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/time/default_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/update_engine_client.h"

using chromeos::DBusThreadManager;
using chromeos::UpdateEngineClient;

namespace {

// How long to wait (each cycle) before checking which severity level we should
// be at. Once we reach the highest severity, the timer will stop.
constexpr base::TimeDelta kNotifyCycleDelta = base::TimeDelta::FromMinutes(20);

// The default amount of time it takes for the detector's annoyance level
// (upgrade_notification_stage()) to reach UPGRADE_ANNOYANCE_HIGH once an
// upgrade is detected.
constexpr base::TimeDelta kDefaultHighThreshold = base::TimeDelta::FromDays(4);

// The scale factor to determine the elevated annoyance level from the high
// annoyance level's threshold. The elevated level always hits half-way to the
// high level.
constexpr double kElevatedScaleFactor = 0.5;

class ChannelsRequester {
 public:
  typedef base::OnceCallback<void(std::string, std::string)>
      OnChannelsReceivedCallback;

  static void Begin(OnChannelsReceivedCallback callback) {
    ChannelsRequester* instance = new ChannelsRequester(std::move(callback));
    UpdateEngineClient* client =
        DBusThreadManager::Get()->GetUpdateEngineClient();
    // base::Unretained is safe because this instance keeps itself alive until
    // both callbacks have run.
    // TODO: use BindOnce here; see https://crbug.com/825993.
    client->GetChannel(true /* get_current_channel */,
                       base::Bind(&ChannelsRequester::SetCurrentChannel,
                                  base::Unretained(instance)));
    client->GetChannel(false /* get_current_channel */,
                       base::Bind(&ChannelsRequester::SetTargetChannel,
                                  base::Unretained(instance)));
  }

 private:
  explicit ChannelsRequester(OnChannelsReceivedCallback callback)
      : callback_(std::move(callback)) {}

  ~ChannelsRequester() = default;

  void SetCurrentChannel(const std::string& current_channel) {
    DCHECK(!current_channel.empty());
    current_channel_ = current_channel;
    TriggerCallbackAndDieIfReady();
  }

  void SetTargetChannel(const std::string& target_channel) {
    DCHECK(!target_channel.empty());
    target_channel_ = target_channel;
    TriggerCallbackAndDieIfReady();
  }

  void TriggerCallbackAndDieIfReady() {
    if (current_channel_.empty() || target_channel_.empty())
      return;
    if (!callback_.is_null()) {
      std::move(callback_).Run(std::move(current_channel_),
                               std::move(target_channel_));
    }
    delete this;
  }

  OnChannelsReceivedCallback callback_;
  std::string current_channel_;
  std::string target_channel_;

  DISALLOW_COPY_AND_ASSIGN(ChannelsRequester);
};

}  // namespace

UpgradeDetectorChromeos::UpgradeDetectorChromeos(
    const base::TickClock* tick_clock)
    : UpgradeDetector(tick_clock),
      high_threshold_(DetermineHighThreshold()),
      upgrade_notification_timer_(tick_clock),
      initialized_(false),
      weak_factory_(this) {}

UpgradeDetectorChromeos::~UpgradeDetectorChromeos() {
}

void UpgradeDetectorChromeos::Init() {
  DBusThreadManager::Get()->GetUpdateEngineClient()->AddObserver(this);
  initialized_ = true;
}

void UpgradeDetectorChromeos::Shutdown() {
  // Init() may not be called from tests.
  if (!initialized_)
    return;
  DBusThreadManager::Get()->GetUpdateEngineClient()->RemoveObserver(this);
  // Discard an outstanding request to a ChannelsRequester.
  weak_factory_.InvalidateWeakPtrs();
  upgrade_notification_timer_.Stop();
  initialized_ = false;
}

base::TimeDelta UpgradeDetectorChromeos::GetHighAnnoyanceLevelDelta() {
  return high_threshold_ - (high_threshold_ * kElevatedScaleFactor);
}

base::TimeTicks UpgradeDetectorChromeos::GetHighAnnoyanceDeadline() {
  const base::TimeTicks detected_time = upgrade_detected_time();
  if (detected_time.is_null())
    return detected_time;
  return detected_time + high_threshold_;
}

// static
base::TimeDelta UpgradeDetectorChromeos::DetermineHighThreshold() {
  base::TimeDelta custom = GetRelaunchNotificationPeriod();
  return custom.is_zero() ? kDefaultHighThreshold : custom;
}

void UpgradeDetectorChromeos::OnRelaunchNotificationPeriodPrefChanged() {
  high_threshold_ = DetermineHighThreshold();
  if (!upgrade_detected_time().is_null())
    NotifyOnUpgrade();
}

void UpgradeDetectorChromeos::UpdateStatusChanged(
    const UpdateEngineClient::Status& status) {
  if (status.status == UpdateEngineClient::UPDATE_STATUS_UPDATED_NEED_REBOOT) {
    set_upgrade_detected_time(tick_clock()->NowTicks());

    ChannelsRequester::Begin(
        base::Bind(&UpgradeDetectorChromeos::OnChannelsReceived,
                   weak_factory_.GetWeakPtr()));
  } else if (status.status ==
             UpdateEngineClient::UPDATE_STATUS_NEED_PERMISSION_TO_UPDATE) {
    // Update engine broadcasts this state only when update is available but
    // downloading over cellular connection requires user's agreement.
    NotifyUpdateOverCellularAvailable();
  }
}

void UpgradeDetectorChromeos::OnUpdateOverCellularOneTimePermissionGranted() {
  NotifyUpdateOverCellularOneTimePermissionGranted();
}

void UpgradeDetectorChromeos::NotifyOnUpgrade() {
  const base::TimeDelta elevated_threshold =
      high_threshold_ * kElevatedScaleFactor;
  base::TimeDelta delta = tick_clock()->NowTicks() - upgrade_detected_time();
  // The delay from now until the next highest notification stage is reached, or
  // zero if the highest notification stage has been reached.
  base::TimeDelta next_delay;

  // These if statements must be sorted (highest interval first).
  if (delta >= high_threshold_) {
    set_upgrade_notification_stage(UPGRADE_ANNOYANCE_HIGH);
  } else if (delta >= elevated_threshold) {
    set_upgrade_notification_stage(UPGRADE_ANNOYANCE_ELEVATED);
    next_delay = high_threshold_ - delta;
  } else {
    set_upgrade_notification_stage(UPGRADE_ANNOYANCE_LOW);
    next_delay = elevated_threshold - delta;
  }

  if (!next_delay.is_zero()) {
    // Schedule the next wakeup in 20 minutes or when the next change to the
    // notification stage should take place.
    upgrade_notification_timer_.Start(
        FROM_HERE, std::min(next_delay, kNotifyCycleDelta), this,
        &UpgradeDetectorChromeos::NotifyOnUpgrade);
  } else if (upgrade_notification_timer_.IsRunning()) {
    // Explicitly stop the timer in case this call is due to a
    // RelaunchNotificationPeriod change that brought the instance up to the
    // "high" annoyance level.
    upgrade_notification_timer_.Stop();
  }

  NotifyUpgrade();
}

void UpgradeDetectorChromeos::OnChannelsReceived(std::string current_channel,
                                                 std::string target_channel) {
  // As current update engine status is UPDATE_STATUS_UPDATED_NEED_REBOOT
  // and target channel is more stable than current channel, powerwash
  // will be performed after reboot.
  set_is_factory_reset_required(UpdateEngineClient::IsTargetChannelMoreStable(
      current_channel, target_channel));

  // ChromeOS shows upgrade arrow once the upgrade becomes available.
  NotifyOnUpgrade();
}

// static
UpgradeDetectorChromeos* UpgradeDetectorChromeos::GetInstance() {
  static base::NoDestructor<UpgradeDetectorChromeos> instance(
      base::DefaultTickClock::GetInstance());
  return instance.get();
}

// static
UpgradeDetector* UpgradeDetector::GetInstance() {
  return UpgradeDetectorChromeos::GetInstance();
}

// static
base::TimeDelta UpgradeDetector::GetDefaultHighAnnoyanceThreshold() {
  return kDefaultHighThreshold;
}
