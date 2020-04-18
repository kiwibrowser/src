// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/critical_notification_bubble_view.h"

#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/prefs/pref_service.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

using base::UserMetricsAction;

namespace {

// How long to give the user until auto-restart if no action is taken. The code
// assumes this to be less than a minute.
const int kCountdownDuration = 30;  // Seconds.

// How often to refresh the bubble UI to update the counter. As long as the
// countdown is in seconds, this should be 1000 or lower.
const int kRefreshBubbleEvery = 1000;  // Millisecond.

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// CriticalNotificationBubbleView

CriticalNotificationBubbleView::CriticalNotificationBubbleView(
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT) {
  set_close_on_deactivate(false);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::CRITICAL_NOTIFICATION);
}

CriticalNotificationBubbleView::~CriticalNotificationBubbleView() {
}

int CriticalNotificationBubbleView::GetRemainingTime() const {
  base::TimeDelta time_lapsed = base::TimeTicks::Now() - bubble_created_;
  return kCountdownDuration - time_lapsed.InSeconds();
}

void CriticalNotificationBubbleView::OnCountdown() {
  UpgradeDetector* upgrade_detector = UpgradeDetector::GetInstance();
  if (upgrade_detector->critical_update_acknowledged()) {
    // The user has already interacted with the bubble and chosen a path.
    GetWidget()->Close();
    return;
  }

  int seconds = GetRemainingTime();
  if (seconds <= 0) {
    // Time's up!
    upgrade_detector->acknowledge_critical_update();

    base::RecordAction(UserMetricsAction("CriticalNotification_AutoRestart"));
    refresh_timer_.Stop();
    chrome::AttemptRestart();
  }

  // Update the counter. It may seem counter-intuitive to update the message
  // after we attempt restart, but remember that shutdown may be aborted by
  // an onbeforeunload handler, leaving the bubble up when the browser should
  // have restarted (giving the user another chance).
  GetBubbleFrameView()->UpdateWindowTitle();
}

base::string16 CriticalNotificationBubbleView::GetWindowTitle() const {
  int seconds = GetRemainingTime();
  return seconds > 0 ? l10n_util::GetPluralStringFUTF16(
                           IDS_CRITICAL_NOTIFICATION_TITLE, seconds)
                     : l10n_util::GetStringUTF16(
                           IDS_CRITICAL_NOTIFICATION_TITLE_ALTERNATE);
}

void CriticalNotificationBubbleView::WindowClosing() {
  refresh_timer_.Stop();
}

bool CriticalNotificationBubbleView::Cancel() {
  UpgradeDetector::GetInstance()->acknowledge_critical_update();
  base::RecordAction(UserMetricsAction("CriticalNotification_Ignore"));
  // If the counter reaches 0, we set a restart flag that must be cleared if
  // the user selects, for example, "Stay on this page" during an
  // onbeforeunload handler.
  PrefService* prefs = g_browser_process->local_state();
  if (prefs->HasPrefPath(prefs::kRestartLastSessionOnShutdown))
    prefs->ClearPref(prefs::kRestartLastSessionOnShutdown);
  return true;
}

bool CriticalNotificationBubbleView::Accept() {
  UpgradeDetector::GetInstance()->acknowledge_critical_update();
  base::RecordAction(UserMetricsAction("CriticalNotification_Restart"));
  chrome::AttemptRestart();
  return true;
}

base::string16 CriticalNotificationBubbleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_CANCEL
                                       ? IDS_CRITICAL_NOTIFICATION_DISMISS
                                       : IDS_CRITICAL_NOTIFICATION_RESTART);
}

void CriticalNotificationBubbleView::Init() {
  bubble_created_ = base::TimeTicks::Now();

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* message = new views::Label();
  message->SetMultiLine(true);
  message->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message->SetText(l10n_util::GetStringUTF16(IDS_CRITICAL_NOTIFICATION_TEXT));
  message->SizeToFit(
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          ChromeDistanceMetric::DISTANCE_BUBBLE_PREFERRED_WIDTH) -
      margins().width());
  AddChildView(message);

  refresh_timer_.Start(FROM_HERE,
      base::TimeDelta::FromMilliseconds(kRefreshBubbleEvery),
      this, &CriticalNotificationBubbleView::OnCountdown);

  base::RecordAction(UserMetricsAction("CriticalNotificationShown"));
}

void CriticalNotificationBubbleView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kAlert;
}

void CriticalNotificationBubbleView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this)
    NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);
}
