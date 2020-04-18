// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/upgrade_detector_impl.h"

#include <stdint.h>

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/build_time.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/process/launch.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/google/google_brand.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/network_time/network_time_tracker.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"

#if defined(OS_WIN)
#include "base/win/win_util.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/install_util.h"
#elif defined(OS_MACOSX)
#include "chrome/browser/mac/keystone_glue.h"
#endif

namespace {

// The default thresholds for reaching annoyance levels.
constexpr base::TimeDelta kDefaultLowThreshold = base::TimeDelta::FromDays(2);
constexpr base::TimeDelta kDefaultElevatedThreshold =
    base::TimeDelta::FromDays(4);
constexpr base::TimeDelta kDefaultHighThreshold = base::TimeDelta::FromDays(7);

// How long (in milliseconds) to wait (each cycle) before checking whether
// Chrome's been upgraded behind our back.
constexpr base::TimeDelta kCheckForUpgrade = base::TimeDelta::FromHours(2);

// How long to wait (each cycle) before checking which severity level we should
// be at. Once we reach the highest severity, the timer will stop.
constexpr base::TimeDelta kNotifyCycleTime = base::TimeDelta::FromMinutes(20);

// Same as kNotifyCycleTimeMs but only used during testing.
constexpr base::TimeDelta kNotifyCycleTimeForTesting =
    base::TimeDelta::FromMilliseconds(500);

// The number of days after which we identify a build/install as outdated.
constexpr base::TimeDelta kOutdatedBuildAge = base::TimeDelta::FromDays(12 * 7);

// Return the string that was passed as a value for the
// kCheckForUpdateIntervalSec switch.
std::string CmdLineInterval() {
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  return cmd_line.GetSwitchValueASCII(switches::kCheckForUpdateIntervalSec);
}

// Check if one of the outdated simulation switches was present on the command
// line.
bool SimulatingOutdated() {
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  return cmd_line.HasSwitch(switches::kSimulateOutdated) ||
      cmd_line.HasSwitch(switches::kSimulateOutdatedNoAU);
}

// Check if any of the testing switches was present on the command line.
bool IsTesting() {
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  return cmd_line.HasSwitch(switches::kSimulateUpgrade) ||
      cmd_line.HasSwitch(switches::kCheckForUpdateIntervalSec) ||
      cmd_line.HasSwitch(switches::kSimulateCriticalUpdate) ||
      SimulatingOutdated();
}

// How often to check for an upgrade.
base::TimeDelta GetCheckForUpgradeDelay() {
  // Check for a value passed via the command line.
  int seconds;
  std::string interval = CmdLineInterval();
  if (!interval.empty() && base::StringToInt(interval, &seconds))
    return base::TimeDelta::FromSeconds(seconds);

  return kCheckForUpgrade;
}

// Return true if the browser is updating on the dev or canary channels.
bool IsUnstableChannel() {
  // Unbranded (Chromium) builds are on the UNKNOWN channel, so check explicitly
  // for the Google Chrome channels that are considered "unstable". This ensures
  // that Chromium builds get the default behavior.
  const version_info::Channel channel = chrome::GetChannel();
  return channel == version_info::Channel::DEV ||
         channel == version_info::Channel::CANARY;
}

// Gets the currently installed version. On Windows, if |critical_update| is not
// NULL, also retrieves the critical update version info if available.
base::Version GetCurrentlyInstalledVersionImpl(base::Version* critical_update) {
  base::AssertBlockingAllowed();

  base::Version installed_version;
#if defined(OS_WIN)
  // Get the version of the currently *installed* instance of Chrome,
  // which might be newer than the *running* instance if we have been
  // upgraded in the background.
  bool system_install = !InstallUtil::IsPerUserInstall();

  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  InstallUtil::GetChromeVersion(dist, system_install, &installed_version);
  if (critical_update && installed_version.IsValid()) {
    InstallUtil::GetCriticalUpdateVersion(dist, system_install,
                                          critical_update);
  }
#elif defined(OS_MACOSX)
  installed_version = base::Version(
      base::UTF16ToASCII(keystone_glue::CurrentlyInstalledVersion()));
#elif defined(OS_POSIX)
  // POSIX but not Mac OS X: Linux, etc.
  base::CommandLine command_line(*base::CommandLine::ForCurrentProcess());
  command_line.AppendSwitch(switches::kProductVersion);
  std::string reply;
  if (!base::GetAppOutput(command_line, &reply)) {
    DLOG(ERROR) << "Failed to get current file version";
    return installed_version;
  }
  base::TrimWhitespaceASCII(reply, base::TRIM_ALL, &reply);

  installed_version = base::Version(reply);
#endif
  return installed_version;
}

}  // namespace

UpgradeDetectorImpl::UpgradeDetectorImpl(const base::TickClock* tick_clock)
    : UpgradeDetector(tick_clock),
      blocking_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
           base::MayBlock()})),
      detect_upgrade_timer_(this->tick_clock()),
      upgrade_notification_timer_(this->tick_clock()),
      is_auto_update_enabled_(true),
      simulating_outdated_(SimulatingOutdated()),
      is_testing_(simulating_outdated_ || IsTesting()),
      build_date_(base::GetBuildTime()),
      weak_factory_(this) {
  InitializeThresholds();
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  // The different command line switches that affect testing can't be used
  // simultaneously, if they do, here's the precedence order, based on the order
  // of the if statements below:
  // - kDisableBackgroundNetworking prevents any of the other command line
  //   switch from being taken into account.
  // - kSimulateUpgrade supersedes critical or outdated upgrade switches.
  // - kSimulateCriticalUpdate has precedence over kSimulateOutdated.
  // - kSimulateOutdatedNoAU has precedence over kSimulateOutdated.
  // - kSimulateOutdated[NoAu] can work on its own, or with a specified date.
  if (cmd_line.HasSwitch(switches::kDisableBackgroundNetworking))
    return;
  if (cmd_line.HasSwitch(switches::kSimulateUpgrade)) {
    UpgradeDetected(UPGRADE_AVAILABLE_REGULAR);
    return;
  }
  if (cmd_line.HasSwitch(switches::kSimulateCriticalUpdate)) {
    UpgradeDetected(UPGRADE_AVAILABLE_CRITICAL);
    return;
  }
  if (simulating_outdated_) {
    // The outdated simulation can work without a value, which means outdated
    // now, or with a value that must be a well formed date/time string that
    // overrides the build date.
    // Also note that to test with a given time/date, until the network time
    // tracking moves off of the VariationsService, the "variations-server-url"
    // command line switch must also be specified for the service to be
    // available on non GOOGLE_CHROME_BUILD.
    std::string switch_name;
    if (cmd_line.HasSwitch(switches::kSimulateOutdatedNoAU)) {
      is_auto_update_enabled_ = false;
      switch_name = switches::kSimulateOutdatedNoAU;
    } else {
      switch_name = switches::kSimulateOutdated;
    }
    std::string build_date = cmd_line.GetSwitchValueASCII(switch_name);
    base::Time maybe_build_time;
    bool result = base::Time::FromString(build_date.c_str(), &maybe_build_time);
    if (result && !maybe_build_time.is_null()) {
      // We got a valid build date simulation so use it and check for upgrades.
      build_date_ = maybe_build_time;
      StartTimerForUpgradeCheck();
    } else {
      // Without a valid date, we simulate that we are already outdated...
      UpgradeDetected(
          is_auto_update_enabled_ ? UPGRADE_NEEDED_OUTDATED_INSTALL
                                  : UPGRADE_NEEDED_OUTDATED_INSTALL_NO_AU);
    }
    return;
  }

  // Register for experiment notifications. Note that since this class is a
  // singleton, it does not need to unregister for notifications when destroyed,
  // since it outlives the VariationsService.
  variations::VariationsService* variations_service =
      g_browser_process->variations_service();
  if (variations_service)
    variations_service->AddObserver(this);

#if defined(OS_WIN)
// Only enable upgrade notifications for Google Chrome builds. Chromium does not
// use an auto-updater.
#if defined(GOOGLE_CHROME_BUILD)
  // There might be a policy/enterprise environment preventing updates, so
  // validate updatability and then call StartTimerForUpgradeCheck
  // appropriately. Skip this step if a past attempt has been made to enable
  // auto updates.
  if (g_browser_process->local_state() &&
      g_browser_process->local_state()->GetBoolean(
          prefs::kAttemptedToEnableAutoupdate)) {
    StartTimerForUpgradeCheck();
  } else {
    base::PostTaskAndReplyWithResult(
        blocking_task_runner_.get(), FROM_HERE,
        base::BindOnce(&GoogleUpdateSettings::AreAutoupdatesEnabled),
        base::BindOnce(&UpgradeDetectorImpl::OnAutoupdatesEnabledResult,
                       weak_factory_.GetWeakPtr()));
  }
#endif  // defined(GOOGLE_CHROME_BUILD)
#else   // defined(OS_WIN)
#if defined(OS_MACOSX)
  // Only enable upgrade notifications if the updater (Keystone) is present.
  if (!keystone_glue::KeystoneEnabled()) {
    is_auto_update_enabled_ = false;
    return;
  }
#elif defined(OS_POSIX)
  // Always enable upgrade notifications regardless of branding.
#else
  return;
#endif
  StartTimerForUpgradeCheck();
#endif  // defined(OS_WIN)
}

UpgradeDetectorImpl::~UpgradeDetectorImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
base::Version UpgradeDetectorImpl::GetCurrentlyInstalledVersion() {
  return GetCurrentlyInstalledVersionImpl(NULL);
}

// static
void UpgradeDetectorImpl::DetectUpgradeTask(
    scoped_refptr<base::TaskRunner> callback_task_runner,
    UpgradeDetectedCallback callback) {
  base::Version critical_update;
  base::Version installed_version =
      GetCurrentlyInstalledVersionImpl(&critical_update);

  // Get the version of the currently *running* instance of Chrome.
  const base::Version& running_version = version_info::GetVersion();
  if (!running_version.IsValid()) {
    NOTREACHED();
    return;
  }

  // |installed_version| may be NULL when the user downgrades on Linux (by
  // switching from dev to beta channel, for example). The user needs a
  // restart in this case as well. See http://crbug.com/46547
  if (!installed_version.IsValid() || installed_version > running_version) {
    // If a more recent version is available, it might be that we are lacking
    // a critical update, such as a zero-day fix.
    UpgradeAvailable upgrade_available = UPGRADE_AVAILABLE_REGULAR;
    if (critical_update.IsValid() && critical_update > running_version)
      upgrade_available = UPGRADE_AVAILABLE_CRITICAL;

    // Fire off the upgrade detected task.
    callback_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), upgrade_available));
  }
}

void UpgradeDetectorImpl::StartTimerForUpgradeCheck() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  detect_upgrade_timer_.Start(FROM_HERE, GetCheckForUpgradeDelay(), this,
                              &UpgradeDetectorImpl::CheckForUpgrade);
}

void UpgradeDetectorImpl::StartUpgradeNotificationTimer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The timer may already be running (e.g. due to both a software upgrade and
  // experiment updates being available).
  if (upgrade_notification_timer_.IsRunning())
    return;

  if (upgrade_detected_time().is_null())
    set_upgrade_detected_time(tick_clock()->NowTicks());

  // Start the repeating timer for notifying the user after a certain period.
  upgrade_notification_timer_.Start(
      FROM_HERE, is_testing_ ? kNotifyCycleTimeForTesting : kNotifyCycleTime,
      this, &UpgradeDetectorImpl::NotifyOnUpgrade);
}

void UpgradeDetectorImpl::InitializeThresholds() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!stages_.empty())
    return;

  DoInitializeThresholds();
#if DCHECK_IS_ON()
  // |stages_| must have at least one element, and must be sorted in decreasing
  // order of time.
  DCHECK(!stages_.empty());
  for (auto scan = stages_.begin() + 1; scan != stages_.end(); ++scan)
    DCHECK_GT((scan - 1)->first, scan->first);

  // elevated_threshold_ must be greater than low_threshold (the last item in)
  // |stages_|.
  DCHECK_GT(elevated_threshold_, stages_.back().first);

  // high_threshold_ must be greater than elevated_threshold_.
  DCHECK_GT(high_threshold_, elevated_threshold_);

  // If elevated_threshold_ and high_threshold_ are present in |stages_|, they
  // must be equal.
  if (stages_.size() != 1) {
    DCHECK(!IsUnstableChannel());
    DCHECK_EQ(stages_.size(), 3U);
    DCHECK_EQ(stages_[1].first, elevated_threshold_);
    DCHECK_EQ(stages_[0].first, high_threshold_);
  } else {
    DCHECK(IsUnstableChannel());
  }
#endif  // DCHECK_IS_ON()
}

void UpgradeDetectorImpl::DoInitializeThresholds() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stages_.empty());

  base::TimeDelta low_threshold;

  // Use a custom notification period for the "high" level, dividing it evenly
  // to set the "low" and "elevated" levels. Such overrides trump all else.
  const base::TimeDelta custom_high = GetRelaunchNotificationPeriod();
  if (!custom_high.is_zero()) {
    low_threshold = custom_high / 3;
    elevated_threshold_ = custom_high - low_threshold;
    high_threshold_ = custom_high;
    // Stages must be sorted by decreasing TimeDelta.
    stages_.emplace_back(high_threshold_, UPGRADE_ANNOYANCE_HIGH);
    stages_.emplace_back(elevated_threshold_, UPGRADE_ANNOYANCE_ELEVATED);
    stages_.emplace_back(low_threshold, UPGRADE_ANNOYANCE_LOW);
    return;
  }

  // Use the default values when no override is set.
  low_threshold = kDefaultLowThreshold;
  elevated_threshold_ = kDefaultElevatedThreshold;
  high_threshold_ = kDefaultHighThreshold;

  // When testing, scale everything back so that a day passes in ten seconds.
  if (is_testing_) {
    static constexpr int64_t scale_factor =
        base::TimeDelta::FromDays(1) / base::TimeDelta::FromSeconds(10);
    low_threshold /= scale_factor;
    elevated_threshold_ /= scale_factor;
    high_threshold_ /= scale_factor;
  }

  // Canary and dev channels are extra special, and reach "low" annoyance after
  // one hour (one second in testing) and never advance beyond that.
  if (IsUnstableChannel()) {
    low_threshold = is_testing_ ? base::TimeDelta::FromSeconds(1)
                                : base::TimeDelta::FromHours(1);
    // High and elevated thresholds are not added to |stages_| on unstable
    // channels.
  } else {
    // Stages must be sorted by decreasing TimeDelta.
    stages_.emplace_back(high_threshold_, UPGRADE_ANNOYANCE_HIGH);
    stages_.emplace_back(elevated_threshold_, UPGRADE_ANNOYANCE_ELEVATED);
  }
  stages_.emplace_back(low_threshold, UPGRADE_ANNOYANCE_LOW);
}

void UpgradeDetectorImpl::CheckForUpgrade() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Interrupt any (unlikely) unfinished execution of DetectUpgradeTask, or at
  // least prevent the callback from being executed, because we will potentially
  // call it from within DetectOutdatedInstall() or will post
  // DetectUpgradeTask again below anyway.
  weak_factory_.InvalidateWeakPtrs();

  // No need to look for upgrades if the install is outdated.
  if (DetectOutdatedInstall())
    return;

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UpgradeDetectorImpl::DetectUpgradeTask,
                     base::SequencedTaskRunnerHandle::Get(),
                     base::BindOnce(&UpgradeDetectorImpl::UpgradeDetected,
                                    weak_factory_.GetWeakPtr())));
}

bool UpgradeDetectorImpl::DetectOutdatedInstall() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  static constexpr base::Feature kOutdatedBuildDetector = {
      "OutdatedBuildDetector", base::FEATURE_ENABLED_BY_DEFAULT};

  if (!base::FeatureList::IsEnabled(kOutdatedBuildDetector))
    return false;

  // Don't show the bubble if we have a brand code that is NOT organic, unless
  // an outdated build is being simulated by command line switches.
  if (!simulating_outdated_) {
    std::string brand;
    if (google_brand::GetBrand(&brand) && !google_brand::IsOrganic(brand))
      return false;

#if defined(OS_WIN)
    // Don't show the update bubbles to enterprise users.
    if (base::win::IsEnterpriseManaged())
      return false;
#endif
  }

  base::Time network_time;
  base::TimeDelta uncertainty;
  if (g_browser_process->network_time_tracker()->GetNetworkTime(&network_time,
                                                                &uncertainty) !=
      network_time::NetworkTimeTracker::NETWORK_TIME_AVAILABLE) {
    // When network time has not been initialized yet, simply rely on the
    // machine's current time.
    network_time = base::Time::Now();
  }

  if (network_time.is_null() || build_date_.is_null() ||
      build_date_ > network_time) {
    NOTREACHED();
    return false;
  }

  if (network_time - build_date_ > kOutdatedBuildAge) {
    UpgradeDetected(is_auto_update_enabled_ ?
        UPGRADE_NEEDED_OUTDATED_INSTALL :
        UPGRADE_NEEDED_OUTDATED_INSTALL_NO_AU);
    return true;
  }
  // If we simlated an outdated install with a date, we don't want to keep
  // checking for version upgrades, which happens on non-official builds.
  return simulating_outdated_;
}

void UpgradeDetectorImpl::UpgradeDetected(UpgradeAvailable upgrade_available) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  set_upgrade_available(upgrade_available);

  // Stop the recurring timer (that is checking for changes).
  detect_upgrade_timer_.Stop();
  set_critical_update_acknowledged(false);

  StartUpgradeNotificationTimer();
}

void UpgradeDetectorImpl::OnExperimentChangesDetected(Severity severity) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  set_best_effort_experiment_updates_available(severity == BEST_EFFORT);
  set_critical_experiment_updates_available(severity == CRITICAL);
  StartUpgradeNotificationTimer();
}

void UpgradeDetectorImpl::NotifyOnUpgradeWithTimePassed(
    base::TimeDelta time_passed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const UpgradeNotificationAnnoyanceLevel last_stage =
      upgrade_notification_stage();

  // Figure out which stage the detector is now in (new_stage) and how far away
  // the next highest stage is (next_delay).
  UpgradeNotificationAnnoyanceLevel new_stage = UPGRADE_ANNOYANCE_NONE;
  base::TimeDelta next_delay;

  if (upgrade_available() > UPGRADE_AVAILABLE_REGULAR ||
      critical_experiment_updates_available()) {
    new_stage = UPGRADE_ANNOYANCE_CRITICAL;
  } else {
    // |stages_| must be sorted by decreasing TimeDelta.
    auto it = std::find_if(stages_.begin(), stages_.end(),
                           [time_passed](const DeltaAndStage& delta_and_stage) {
                             return time_passed >= delta_and_stage.first;
                           });
    if (it != stages_.end())
      new_stage = it->second;
    if (it != stages_.begin())
      next_delay = (it - 1)->first - time_passed;
  }

  set_upgrade_notification_stage(new_stage);
  if (!next_delay.is_zero()) {
    // Schedule the next wakeup in 20 minutes or when the next change to the
    // notification stage should take place.
    upgrade_notification_timer_.Start(
        FROM_HERE,
        std::min(next_delay,
                 is_testing_ ? kNotifyCycleTimeForTesting : kNotifyCycleTime),
        this, &UpgradeDetectorImpl::NotifyOnUpgrade);
  } else if (upgrade_notification_timer_.IsRunning()) {
    // Explicitly stop the timer in case this call is due to a change (e.g., in
    // the RelaunchNotificationPeriod) that brought the instance up to or above
    // the "high" annoyance level.
    upgrade_notification_timer_.Stop();
  }

  // Issue a notification if the stage is above "none" or if it's dropped down
  // to "none" from something higher.
  if (new_stage != UPGRADE_ANNOYANCE_NONE ||
      last_stage != UPGRADE_ANNOYANCE_NONE) {
    NotifyUpgrade();
  }
}

base::TimeDelta UpgradeDetectorImpl::GetThresholdForLevel(
    UpgradeNotificationAnnoyanceLevel level) {
  DCHECK(!stages_.empty());
  switch (level) {
    case UPGRADE_ANNOYANCE_LOW:
      // Low is always the last item in |stages_|.
      return stages_.back().first;
    case UPGRADE_ANNOYANCE_ELEVATED:
      // Elevated is not present in |stages_| on unstable channels.
      return elevated_threshold_;
    case UPGRADE_ANNOYANCE_HIGH:
      break;
    case UPGRADE_ANNOYANCE_NONE:
    case UPGRADE_ANNOYANCE_CRITICAL:
      NOTREACHED();
      break;
  }
  // High is not present in |stages_| on unstable channels.
  return high_threshold_;
}

void UpgradeDetectorImpl::OnRelaunchNotificationPeriodPrefChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Force a recomputation of the thresholds.
  stages_.clear();
  InitializeThresholds();

  // Broadcast the appropriate notification if an upgrade has been detected.
  if (upgrade_available() != UPGRADE_AVAILABLE_NONE)
    NotifyOnUpgrade();
}

#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
void UpgradeDetectorImpl::OnAutoupdatesEnabledResult(
    bool auto_updates_enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  is_auto_update_enabled_ = auto_updates_enabled;
  StartTimerForUpgradeCheck();
}
#endif  // defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)

void UpgradeDetectorImpl::NotifyOnUpgrade() {
  const base::TimeDelta time_passed =
      tick_clock()->NowTicks() - upgrade_detected_time();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NotifyOnUpgradeWithTimePassed(time_passed);
}

// static
UpgradeDetectorImpl* UpgradeDetectorImpl::GetInstance() {
  static base::NoDestructor<UpgradeDetectorImpl> instance(
      base::DefaultTickClock::GetInstance());
  return instance.get();
}

base::TimeDelta UpgradeDetectorImpl::GetHighAnnoyanceLevelDelta() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Elevated and high thresholds are not present in |stages_| for unstable
  // channels, but their delta is still valid in such case.
  return high_threshold_ - elevated_threshold_;
}

base::TimeTicks UpgradeDetectorImpl::GetHighAnnoyanceDeadline() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const base::TimeTicks detected_time = upgrade_detected_time();
  if (detected_time.is_null())
    return detected_time;
  // While dev and canary will never reach high annoyance (see comment in
  // NotifyOnUpgradeWithTimePassed), they do obey its default deadline.
  return detected_time + high_threshold_;
}

// static
UpgradeDetector* UpgradeDetector::GetInstance() {
  return UpgradeDetectorImpl::GetInstance();
}

// static
base::TimeDelta UpgradeDetector::GetDefaultHighAnnoyanceThreshold() {
  return kDefaultHighThreshold;
}
