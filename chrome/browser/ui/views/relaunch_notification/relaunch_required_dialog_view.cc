// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/relaunch_notification/relaunch_required_dialog_view.h"

#include <cmath>

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

namespace {

constexpr int kTitleIconSize = 20;

}  // namespace

// static
views::Widget* RelaunchRequiredDialogView::Show(
    Browser* browser,
    base::TimeTicks deadline,
    base::RepeatingClosure on_accept) {
  views::Widget* widget = constrained_window::CreateBrowserModalDialogViews(
      new RelaunchRequiredDialogView(deadline, std::move(on_accept)),
      browser->window()->GetNativeWindow());
  widget->Show();
  return widget;
}

RelaunchRequiredDialogView::~RelaunchRequiredDialogView() = default;

// static
RelaunchRequiredDialogView* RelaunchRequiredDialogView::FromWidget(
    views::Widget* widget) {
  return static_cast<RelaunchRequiredDialogView*>(
      widget->widget_delegate()->AsDialogDelegate());
}

void RelaunchRequiredDialogView::SetDeadline(base::TimeTicks deadline) {
  if (deadline != relaunch_deadline_) {
    relaunch_deadline_ = deadline;
    // Refresh the title immediately.
    OnTitleRefresh();
  }
}

bool RelaunchRequiredDialogView::Accept() {
  base::RecordAction(base::UserMetricsAction("RelaunchRequired_Accept"));

  on_accept_.Run();

  // Keep the dialog open in case shutdown is prevented for some reason so that
  // the user can try again if needed.
  return false;
}

bool RelaunchRequiredDialogView::Close() {
  base::RecordAction(base::UserMetricsAction("RelaunchRequired_Close"));

  return true;
}

base::string16 RelaunchRequiredDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK
                                       ? IDS_RELAUNCH_ACCEPT_BUTTON
                                       : IDS_RELAUNCH_REQUIRED_CANCEL_BUTTON);
}

ui::ModalType RelaunchRequiredDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 RelaunchRequiredDialogView::GetWindowTitle() const {
  // Round the time-to-relaunch to the nearest "boundary", which may be a day,
  // hour, minute, or second. For example, two days and eighteen hours will be
  // rounded up to three days, while two days and one hour will be rounded down
  // to two days. This rounding is significant for only the initial showing of
  // the dialog. Each refresh of the title thereafter will take place at the
  // moment when the boundary value changes. For example, the title will be
  // refreshed from three days to two days when there are exactly two days
  // remaning. This scales nicely to the final seconds, when one would expect a
  // "3..2..1.." countdown to change precisely on the per-second boundaries.
  const base::TimeDelta rounded_offset =
      ComputeDeadlineDelta(relaunch_deadline_ - base::TimeTicks::Now());

  int amount = rounded_offset.InSeconds();
  int message_id = IDS_RELAUNCH_REQUIRED_TITLE_SECONDS;
  if (rounded_offset.InDays() >= 2) {
    amount = rounded_offset.InDays();
    message_id = IDS_RELAUNCH_REQUIRED_TITLE_DAYS;
  } else if (rounded_offset.InHours() >= 1) {
    amount = rounded_offset.InHours();
    message_id = IDS_RELAUNCH_REQUIRED_TITLE_HOURS;
  } else if (rounded_offset.InMinutes() >= 1) {
    amount = rounded_offset.InMinutes();
    message_id = IDS_RELAUNCH_REQUIRED_TITLE_MINUTES;
  }

  return l10n_util::GetPluralStringFUTF16(message_id, amount);
}

bool RelaunchRequiredDialogView::ShouldShowCloseButton() const {
  return false;
}

gfx::ImageSkia RelaunchRequiredDialogView::GetWindowIcon() {
  return gfx::CreateVectorIcon(gfx::IconDescription(
      vector_icons::kBusinessIcon, kTitleIconSize, gfx::kChromeIconGrey,
      base::TimeDelta(), gfx::kNoneIcon));
}

bool RelaunchRequiredDialogView::ShouldShowWindowIcon() const {
  return true;
}

int RelaunchRequiredDialogView::GetHeightForWidth(int width) const {
  const gfx::Insets insets = GetInsets();
  return body_label_->GetHeightForWidth(width - insets.width()) +
         insets.height();
}

void RelaunchRequiredDialogView::Layout() {
  body_label_->SetBoundsRect(GetContentsBounds());
}

// static
base::TimeDelta RelaunchRequiredDialogView::ComputeDeadlineDelta(
    base::TimeDelta deadline_offset) {
  // Round deadline_offset to the nearest second for the computations below.
  deadline_offset =
      base::TimeDelta::FromSeconds(std::round(deadline_offset.InSecondsF()));

  // At/above 47.5 hours, round up to showing N days (min 2).
  // TODO(grt): Explore ways to make this more obvious by way of new methods in
  // base::TimeDelta (e.g., float variants of FromXXX and rounding variants of
  // InXXX).
  static constexpr base::TimeDelta kMinDaysDelta =
      base::TimeDelta::FromMinutes(47 * 60 + 30);
  // At/above 59.5 minutes, round up to showing N hours (min 1).
  static constexpr base::TimeDelta kMinHoursDelta =
      base::TimeDelta::FromSeconds(59 * 60 + 30);
  // At/above 59.5 seconds, round up to showing N minutes (min 1).
  static constexpr base::TimeDelta kMinMinutesDelta =
      base::TimeDelta::FromMilliseconds(59 * 1000 + 500);

  // Round based on the time scale.
  if (deadline_offset >= kMinDaysDelta) {
    return base::TimeDelta::FromDays(
        (deadline_offset + base::TimeDelta::FromHours(12)).InDays());
  }

  if (deadline_offset >= kMinHoursDelta) {
    return base::TimeDelta::FromHours(
        (deadline_offset + base::TimeDelta::FromMinutes(30)).InHours());
  }

  if (deadline_offset >= kMinMinutesDelta) {
    return base::TimeDelta::FromMinutes(
        (deadline_offset + base::TimeDelta::FromSeconds(30)).InMinutes());
  }

  return base::TimeDelta::FromSeconds(
      (deadline_offset + base::TimeDelta::FromMilliseconds(500)).InSeconds());
}

// static
base::TimeDelta RelaunchRequiredDialogView::ComputeNextRefreshDelta(
    base::TimeDelta deadline_offset) {
  // What would be in the title now?
  const base::TimeDelta rounded_offset = ComputeDeadlineDelta(deadline_offset);

  // Compute the refresh moment to bring |rounded_offset| down to the next value
  // to be displayed. This is the moment that the title must switch from N to
  // N-1 of the same units (e.g., # of days) or from one form of units to the
  // next granular form of units (e.g., 2 days to 47 hours).
  // TODO(grt): Find a way to reduce duplication with the constants in
  // ComputeDeadlineDelta once https://crbug.com/761570 is resolved.
  static constexpr base::TimeDelta kMinDays = base::TimeDelta::FromDays(2);
  static constexpr base::TimeDelta kMinHours = base::TimeDelta::FromHours(1);
  static constexpr base::TimeDelta kMinMinutes =
      base::TimeDelta::FromMinutes(1);
  static constexpr base::TimeDelta kMinSeconds =
      base::TimeDelta::FromSeconds(1);

  base::TimeDelta delta;
  if (rounded_offset > kMinDays)
    delta = base::TimeDelta::FromDays(rounded_offset.InDays() - 1);
  else if (rounded_offset > kMinHours)
    delta = base::TimeDelta::FromHours(rounded_offset.InHours() - 1);
  else if (rounded_offset > kMinMinutes)
    delta = base::TimeDelta::FromMinutes(rounded_offset.InMinutes() - 1);
  else if (rounded_offset > kMinSeconds)
    delta = base::TimeDelta::FromSeconds(rounded_offset.InSeconds() - 1);

  return deadline_offset - delta;
}

gfx::Size RelaunchRequiredDialogView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

RelaunchRequiredDialogView::RelaunchRequiredDialogView(
    base::TimeTicks deadline,
    base::RepeatingClosure on_accept)
    : relaunch_deadline_(deadline),
      on_accept_(on_accept),
      body_label_(nullptr),
      last_refresh_(false) {
  chrome::RecordDialogCreation(chrome::DialogIdentifier::RELAUNCH_REQUIRED);
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));

  body_label_ =
      new views::Label(l10n_util::GetStringUTF16(IDS_RELAUNCH_REQUIRED_BODY),
                       views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT);
  body_label_->SetMultiLine(true);
  body_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Align the body label with the left edge of the dialog's title.
  // TODO(bsep): Remove this when fixing https://crbug.com/810970.
  int title_offset = 2 * views::LayoutProvider::Get()
                             ->GetInsetsMetric(views::INSETS_DIALOG_TITLE)
                             .left() +
                     kTitleIconSize;
  body_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets(0, title_offset - margins().left(), 0, 0)));

  AddChildView(body_label_);

  base::RecordAction(base::UserMetricsAction("RelaunchRequiredShown"));

  // Start the timer for the next time the title neeeds to be updated (e.g.,
  // from "2 days" to "3 days").
  ScheduleNextTitleRefresh();
}

void RelaunchRequiredDialogView::ScheduleNextTitleRefresh() {
  // Refresh at the next second, minute, hour, or day boundary; depending on the
  // relaunch deadline.
  const base::TimeDelta deadline_offset =
      relaunch_deadline_ - base::TimeTicks::Now();
  const base::TimeDelta refresh_delta =
      ComputeNextRefreshDelta(deadline_offset);

  // Note if this is the last refresh.
  if (refresh_delta == deadline_offset)
    last_refresh_ = true;

  refresh_timer_.Start(FROM_HERE, refresh_delta, this,
                       &RelaunchRequiredDialogView::OnTitleRefresh);
}

void RelaunchRequiredDialogView::OnTitleRefresh() {
  GetBubbleFrameView()->UpdateWindowTitle();
  if (!last_refresh_)
    ScheduleNextTitleRefresh();
}

views::BubbleFrameView* RelaunchRequiredDialogView::GetBubbleFrameView() {
  const views::NonClientView* view =
      GetWidget() ? GetWidget()->non_client_view() : nullptr;
  return view ? static_cast<views::BubbleFrameView*>(view->frame_view())
              : nullptr;
}
