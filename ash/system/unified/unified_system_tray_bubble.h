// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_BUBBLE_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_BUBBLE_H_

#include <memory>

#include "ash/system/tray/time_to_click_recorder.h"
#include "ash/system/tray/tray_bubble_base.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Widget;
}

namespace ash {

class UnifiedSystemTray;
class UnifiedSystemTrayController;

// Manages the bubble that contains UnifiedSystemTrayView.
// Shows the bubble on the constructor, and closes the bubble on the destructor.
// It is possible that the bubble widget is closed on deactivation. In such
// case, this class calls UnifiedSystemTray::CloseBubble() to delete itself.
class UnifiedSystemTrayBubble : public TrayBubbleBase,
                                public views::WidgetObserver,
                                public TimeToClickRecorder::Delegate {
 public:
  explicit UnifiedSystemTrayBubble(UnifiedSystemTray* tray, bool show_by_click);
  ~UnifiedSystemTrayBubble() override;

  // Return the bounds of the bubble in the screen.
  gfx::Rect GetBoundsInScreen() const;

  // TrayBubbleBase:
  TrayBackgroundView* GetTray() const override;
  views::TrayBubbleView* GetBubbleView() const override;
  views::Widget* GetBubbleWidget() const override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // TimeToClickRecorder::Delegate:
  void RecordTimeToClick() override;

 private:
  // Controller of UnifiedSystemTrayView. As the view is owned by views
  // hierarchy, we have to own the controller here.
  std::unique_ptr<UnifiedSystemTrayController> controller_;

  // Owner of this class.
  UnifiedSystemTray* tray_;

  // Widget that contains UnifiedSystemTrayView. Unowned.
  // When the widget is closed by deactivation, |bubble_widget_| pointer is
  // invalidated and we have to delete UnifiedSystemTrayBubble by calling
  // UnifiedSystemTray::CloseBubble().
  // In order to do this, we observe OnWidgetDestroying().
  views::Widget* bubble_widget_ = nullptr;

  // PreTargetHandler of |unified_view_| to record TimeToClick metrics. Owned.
  std::unique_ptr<TimeToClickRecorder> time_to_click_recorder_;

  // The time the bubble is created. If the bubble is not created by button
  // click (|show_by_click| in ctor is false), it is not set.
  base::Optional<base::TimeTicks> time_shown_by_click_;

  views::TrayBubbleView* bubble_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTrayBubble);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_BUBBLE_H_
