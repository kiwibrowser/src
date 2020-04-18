// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

class Browser;

namespace views {
class Button;
class Label;
class Widget;
}  // namespace views

// A View for the relaunch recommended bubble. This is shown to users to
// encourage them to relaunch Chrome by the RelaunchNotificationController as
// dictated by policy settings and upgrade availability.
class RelaunchRecommendedBubbleView : public LocationBarBubbleDelegateView {
 public:
  // Shows the bubble in |browser| for an upgrade that was detected at
  // |detection_time|. |on_accept| is run if the user accepts the prompt to
  // restart.
  static views::Widget* ShowBubble(Browser* browser,
                                   base::TimeTicks detection_time,
                                   base::RepeatingClosure on_accept);
  ~RelaunchRecommendedBubbleView() override;

  // LocationBarBubbleDelegateView:
  bool Accept() override;
  bool Close() override;
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;
  int GetHeightForWidth(int width) const override;
  void Layout() override;

 protected:
  // LocationBarBubbleDelegateView:
  void Init() override;
  gfx::Size CalculatePreferredSize() const override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;

 private:
  RelaunchRecommendedBubbleView(views::Button* anchor_button,
                                const gfx::Point& anchor_point,
                                base::TimeTicks detection_time,
                                base::RepeatingClosure on_accept);

  // Schedules a timer to fire the next time the title text must be updated; for
  // example, from "...is available" to "...has been available for 1 day".
  void ScheduleNextTitleRefresh();

  // Invoked when the timer fires to refresh the title text.
  void OnTitleRefresh();

  // The tick count at which Chrome noticed that an update was available. This
  // is used to write the proper string into the dialog's title and to schedule
  // title refreshes to update said string.
  const base::TimeTicks detection_time_;

  // A callback run if the user accepts the prompt to relaunch the browser.
  base::RepeatingClosure on_accept_;

  // The label containing the body text of the bubble.
  views::Label* body_label_;

  // A timer with which title refreshes are scheduled.
  base::OneShotTimer refresh_timer_;

  DISALLOW_COPY_AND_ASSIGN(RelaunchRecommendedBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_
