// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/relaunch_notification/relaunch_notification_controller.h"

#include <algorithm>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/relaunch_notification/relaunch_recommended_bubble_view.h"
#include "chrome/browser/ui/views/relaunch_notification/relaunch_required_dialog_view.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/views/widget/widget.h"

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/background_mode_manager.h"
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)

namespace {

constexpr char kShowResultHistogramPrefix[] = "RelaunchNotification.ShowResult";

// A type represending the possible RelaunchNotification policy setting values.
enum class RelaunchNotificationSetting {
  // Indications are via the Chrome menu only -- no work for the controller.
  kChromeMenuOnly,

  // Present the relaunch recommended bubble in the last active browser window.
  kRecommendedBubble,

  // Present the relaunch required dialog in the last active browser window.
  kRequiredDialog,
};

// Returns the policy setting, mapping out-of-range values to kChromeMenuOnly.
RelaunchNotificationSetting ReadPreference() {
  switch (g_browser_process->local_state()->GetInteger(
      prefs::kRelaunchNotification)) {
    case 1:
      return RelaunchNotificationSetting::kRecommendedBubble;
    case 2:
      return RelaunchNotificationSetting::kRequiredDialog;
  }
  return RelaunchNotificationSetting::kChromeMenuOnly;
}

// The result of an attempt to show a relaunch notification dialog. These values
// are persisted to logs. Entries should not be renumbered and numeric values
// should never be reused.
enum class ShowResult {
  kShown = 0,
  kUnknownNotShownReason = 1,
  kBackgroundModeNoWindows = 2,
  kCount
};

// Returns the reason why a dialog was not shown when the conditions were ripe
// for such.
ShowResult GetNotShownReason() {
#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  BackgroundModeManager* background_mode_manager =
      g_browser_process->background_mode_manager();
  if (background_mode_manager &&
      background_mode_manager->IsBackgroundWithoutWindows()) {
    return ShowResult::kBackgroundModeNoWindows;
  }
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)
  return ShowResult::kUnknownNotShownReason;
}

// Returns the last active tabbed browser.
Browser* FindLastActiveTabbedBrowser() {
  BrowserList* browser_list = BrowserList::GetInstance();
  const auto end = browser_list->end_last_active();
  for (auto scan = browser_list->begin_last_active(); scan != end; ++scan) {
    if ((*scan)->is_type_tabbed())
      return *scan;
  }
  return nullptr;
}

}  // namespace

RelaunchNotificationController::RelaunchNotificationController(
    UpgradeDetector* upgrade_detector)
    : RelaunchNotificationController(upgrade_detector,
                                     base::DefaultTickClock::GetInstance()) {}

RelaunchNotificationController::~RelaunchNotificationController() {
  if (last_notification_style_ != NotificationStyle::kNone)
    StopObservingUpgrades();
}

// static
constexpr base::TimeDelta RelaunchNotificationController::kRelaunchGracePeriod;

RelaunchNotificationController::RelaunchNotificationController(
    UpgradeDetector* upgrade_detector,
    const base::TickClock* tick_clock)
    : upgrade_detector_(upgrade_detector),
      tick_clock_(tick_clock),
      last_notification_style_(NotificationStyle::kNone),
      last_level_(UpgradeDetector::UPGRADE_ANNOYANCE_NONE),
      widget_(nullptr),
      timer_(tick_clock_) {
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    pref_change_registrar_.Init(local_state);
    // base::Unretained is safe here because |this| outlives the registrar.
    pref_change_registrar_.Add(
        prefs::kRelaunchNotification,
        base::BindRepeating(&RelaunchNotificationController::HandleCurrentStyle,
                            base::Unretained(this)));
    // Synchronize the instance with the current state of the preference.
    HandleCurrentStyle();
  }
}

void RelaunchNotificationController::OnUpgradeRecommended() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  UpgradeDetector::UpgradeNotificationAnnoyanceLevel current_level =
      upgrade_detector_->upgrade_notification_stage();
  const base::TimeTicks current_high_deadline =
      upgrade_detector_->GetHighAnnoyanceDeadline();

  // Nothing to do if there has been no change in the level and deadline. If
  // appropriate, a notification for this level has already been shown.
  if (current_level == last_level_ &&
      current_high_deadline == last_high_deadline_) {
    return;
  }

  switch (current_level) {
    case UpgradeDetector::UPGRADE_ANNOYANCE_NONE:
      // While it's unexpected that the level could move back down to none, it's
      // not a challenge to do the right thing.
      CloseRelaunchNotification();
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_LOW:
    case UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED:
    case UpgradeDetector::UPGRADE_ANNOYANCE_HIGH:
      ShowRelaunchNotification(current_level, current_high_deadline);
      break;
    case UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL:
      // Critical notifications are handled by ToolbarView.
      // TODO(grt): Reconsider this when implementing the relaunch required
      // dialog. Obeying the administrator's wish to force a relaunch when
      // the annoyance level reaches HIGH is more important than showing the
      // critical update dialog. Perhaps handling of "critical" events should
      // be decoupled from the "relaunch to update" events.
      CloseRelaunchNotification();
      break;
  }

  last_level_ = current_level;
  last_high_deadline_ = current_high_deadline;
}

void RelaunchNotificationController::OnWidgetClosing(views::Widget* widget) {
  DCHECK_EQ(widget, widget_);
  widget->RemoveObserver(this);
  widget_ = nullptr;
}

std::string RelaunchNotificationController::GetSuffixedHistogramName(
    base::StringPiece base_name) {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);
  return base_name.as_string().append(last_notification_style_ ==
                                              NotificationStyle::kRecommended
                                          ? ".Recommended"
                                          : ".Required");
}

void RelaunchNotificationController::HandleCurrentStyle() {
  NotificationStyle notification_style = NotificationStyle::kNone;

  switch (ReadPreference()) {
    case RelaunchNotificationSetting::kChromeMenuOnly:
      DCHECK_EQ(notification_style, NotificationStyle::kNone);
      break;
    case RelaunchNotificationSetting::kRecommendedBubble:
      notification_style = NotificationStyle::kRecommended;
      break;
    case RelaunchNotificationSetting::kRequiredDialog:
      notification_style = NotificationStyle::kRequired;
      break;
  }

  // Nothing to do if there has been no change in the preference.
  if (notification_style == last_notification_style_)
    return;

  // Close the bubble or dialog if either is open.
  if (last_notification_style_ != NotificationStyle::kNone)
    CloseRelaunchNotification();

  // Reset state so that a notifications is shown anew in a new style if needed.
  last_level_ = UpgradeDetector::UPGRADE_ANNOYANCE_NONE;

  if (notification_style == NotificationStyle::kNone) {
    // Transition away from monitoring for upgrade events back to being dormant:
    // there is no need since AppMenuIconController takes care of updating the
    // Chrome menu as needed.
    StopObservingUpgrades();
    last_notification_style_ = notification_style;
    return;
  }

  // Transitioning away from being dormant: observe the UpgradeDetector.
  if (last_notification_style_ == NotificationStyle::kNone)
    StartObservingUpgrades();

  last_notification_style_ = notification_style;

  // Synchronize the instance with the current state of detection.
  OnUpgradeRecommended();
}

void RelaunchNotificationController::StartObservingUpgrades() {
  upgrade_detector_->AddObserver(this);
}

void RelaunchNotificationController::StopObservingUpgrades() {
  upgrade_detector_->RemoveObserver(this);
}

void RelaunchNotificationController::ShowRelaunchNotification(
    UpgradeDetector::UpgradeNotificationAnnoyanceLevel level,
    base::TimeTicks high_deadline) {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);

  if (last_notification_style_ == NotificationStyle::kRecommended) {
    // If this is the final showing (the one at the "high" level), start the
    // timer to reshow the bubble at each "elevated to high" interval.
    if (level == UpgradeDetector::UPGRADE_ANNOYANCE_HIGH) {
      StartReshowTimer();
    } else {
      // Make sure the timer isn't running following a drop down from HIGH to a
      // lower level.
      timer_.Stop();
    }

    // Show the dialog if there has been a level change.
    if (level != last_level_)
      ShowRelaunchRecommendedBubble();
  } else {
    HandleRelaunchRequiredState(level, high_deadline);
  }
}

void RelaunchNotificationController::CloseRelaunchNotification() {
  DCHECK_NE(last_notification_style_, NotificationStyle::kNone);

  // Nothing needs to be closed if the annoyance level is none or critical.
  if (last_level_ == UpgradeDetector::UPGRADE_ANNOYANCE_NONE ||
      last_level_ == UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL) {
    return;
  }

  // Explicit closure cancels either repeatedly reshowing the bubble or the
  // forced relaunch.
  timer_.Stop();

  CloseWidget();
}

void RelaunchNotificationController::HandleRelaunchRequiredState(
    UpgradeDetector::UpgradeNotificationAnnoyanceLevel level,
    base::TimeTicks high_deadline) {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRequired);

  // Make no changes if the new deadline is not in the future and the browser is
  // within the grace period of the previous deadline. The user has already been
  // given the three-minute countdown so just let it go.
  const base::TimeTicks now = tick_clock_->NowTicks();
  if (timer_.IsRunning()) {
    const base::TimeTicks& desired_run_time = timer_.desired_run_time();
    DCHECK(!desired_run_time.is_null());
    if (high_deadline <= now && desired_run_time - now <= kRelaunchGracePeriod)
      return;
  }

  // Compute the new deadline (minimally three minutes into the future).
  const base::TimeTicks deadline =
      std::max(high_deadline, now) + kRelaunchGracePeriod;

  // (re)Start the timer to perform the relaunch when the deadline is reached.
  timer_.Start(FROM_HERE, deadline - now, this,
               &RelaunchNotificationController::OnRelaunchDeadlineExpired);

  if (widget_) {
    // Tell the dialog to update its title if it is showing.
    RelaunchRequiredDialogView::FromWidget(widget_)->SetDeadline(deadline);
  } else {
    // Otherwise, show the dialog if there has been a level change or if the
    // deadline is in the past.
    if (level != last_level_ || high_deadline <= now)
      ShowRelaunchRequiredDialog();
  }
}

void RelaunchNotificationController::StartReshowTimer() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRecommended);
  base::TimeDelta delay = upgrade_detector_->GetHighAnnoyanceLevelDelta();
  if (timer_.IsRunning()) {
    // Leave well enough alone if the timer is already running with the proper
    // frequency. This should not happen given the early-exit in
    // OnUpgradeRecommended when there is no change in the annoyance level or
    // high annoyance deadline.
    if (timer_.GetCurrentDelay() == delay)
      return;

    // Compute the new delay to have the previously-scheduled reshow appear at
    // the right time.
    DCHECK(!timer_.desired_run_time().is_null());
    const base::TimeTicks start_time =
        timer_.desired_run_time() - timer_.GetCurrentDelay();
    const base::TimeTicks new_run_time = start_time + delay;
    const base::TimeTicks now = tick_clock_->NowTicks();
    if (new_run_time <= now) {
      // The new delay puts the next reshow in the past. Handle it now.
      timer_.Stop();
      OnReshowRelaunchRecommendedBubble();
      return;
    }
    delay = new_run_time - now;
  }
  timer_.Start(
      FROM_HERE, delay, this,
      &RelaunchNotificationController::OnReshowRelaunchRecommendedBubble);
}

void RelaunchNotificationController::OnReshowRelaunchRecommendedBubble() {
  DCHECK_EQ(last_notification_style_, NotificationStyle::kRecommended);
  ShowRelaunchRecommendedBubble();
  StartReshowTimer();
}

void RelaunchNotificationController::ShowRelaunchRecommendedBubble() {
  // Nothing to do if the bubble is visible.
  if (widget_)
    return;

  // Show the bubble in the most recently active browser.
  Browser* browser = FindLastActiveTabbedBrowser();
  base::UmaHistogramEnumeration(
      GetSuffixedHistogramName(kShowResultHistogramPrefix),
      browser ? ShowResult::kShown : GetNotShownReason(), ShowResult::kCount);
  if (!browser)
    return;

  widget_ = RelaunchRecommendedBubbleView::ShowBubble(
      browser, upgrade_detector_->upgrade_detected_time(),
      base::BindRepeating(&chrome::AttemptRestart));

  // Monitor the widget so that |widget_| can be cleared on close.
  widget_->AddObserver(this);
}

void RelaunchNotificationController::ShowRelaunchRequiredDialog() {
  DCHECK(timer_.IsRunning());
  DCHECK(!timer_.desired_run_time().is_null());

  // Nothing to do if the dialog is visible.
  if (widget_)
    return;

  // Show the dialog in the most recently active browser.
  Browser* browser = FindLastActiveTabbedBrowser();
  base::UmaHistogramEnumeration(
      GetSuffixedHistogramName(kShowResultHistogramPrefix),
      browser ? ShowResult::kShown : GetNotShownReason(), ShowResult::kCount);
  if (!browser)
    return;

  widget_ = RelaunchRequiredDialogView::Show(
      browser, timer_.desired_run_time(),
      base::BindRepeating(&chrome::AttemptRestart));

  // Monitor the widget so that |widget_| can be cleared on close.
  widget_->AddObserver(this);
}

void RelaunchNotificationController::CloseWidget() {
  if (widget_)
    widget_->Close();
}

void RelaunchNotificationController::OnRelaunchDeadlineExpired() {
  chrome::AttemptRestart();
}
