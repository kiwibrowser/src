// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_REQUIRED_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_REQUIRED_DIALOG_VIEW_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/views/window/dialog_delegate.h"

class Browser;
namespace views {
class BubbleFrameView;
class Label;
class Widget;
}  // namespace views

// A View for the relaunch required dialog. This is shown to users to inform
// them that Chrome will be relaunched by the RelaunchNotificationController as
// dictated by policy settings and upgrade availability.
class RelaunchRequiredDialogView : views::DialogDelegateView {
 public:
  // Shows the dialog in |browser| for a relaunch that will be forced at
  // |deadline|. |on_accept| is run if the user accepts the prompt to restart.
  static views::Widget* Show(Browser* browser,
                             base::TimeTicks deadline,
                             base::RepeatingClosure on_accept);

  ~RelaunchRequiredDialogView() override;

  // Returns the instance hosted by |widget|. |widget| must be an instance
  // previously returned from Show().
  static RelaunchRequiredDialogView* FromWidget(views::Widget* widget);

  // Sets the relaunch deadline to |deadline| and refreshes the view's title
  // accordingly.
  void SetDeadline(base::TimeTicks deadline);

  // views::DialogDelegateView:
  bool Accept() override;
  bool Close() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;
  int GetHeightForWidth(int width) const override;
  void Layout() override;

  // Rounds |deadline_offset| to the nearest day/hour/minute/second for display
  // in the dialog's title.
  static base::TimeDelta ComputeDeadlineDelta(base::TimeDelta deadline_offset);

  // Returns the offset from an arbitrary "now" into |deadline_offset| at which
  // the view's title must be refreshed.
  static base::TimeDelta ComputeNextRefreshDelta(
      base::TimeDelta deadline_offset);

 protected:
  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;

 private:
  RelaunchRequiredDialogView(base::TimeTicks deadline,
                             base::RepeatingClosure on_accept);

  // Schedules a timer to fire the next time the title must be updated.
  void ScheduleNextTitleRefresh();

  // Invoked when the timer fires to refresh the title text.
  void OnTitleRefresh();

  // Returns the containing widget's NonClientView's FrameView as a
  // BubbleFrameView.
  views::BubbleFrameView* GetBubbleFrameView();

  // The time at which Chrome will be forcefully relaunched.
  base::TimeTicks relaunch_deadline_;

  // A callback to run if the user accepts the prompt to relaunch the browser.
  base::RepeatingClosure on_accept_;

  // The label containing the body text of the dialog.
  views::Label* body_label_;

  // A timer with which title refreshes are scheduled.
  base::OneShotTimer refresh_timer_;

  // Becomes true upon the last title refresh, at which point no more refreshes
  // are necessary.
  bool last_refresh_;

  DISALLOW_COPY_AND_ASSIGN(RelaunchRequiredDialogView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_REQUIRED_DIALOG_VIEW_H_
