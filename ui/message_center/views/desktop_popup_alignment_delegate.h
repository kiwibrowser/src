// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_DESKTOP_POPUP_ALIGNMENT_DELEGATE_H_
#define UI_MESSAGE_CENTER_VIEWS_DESKTOP_POPUP_ALIGNMENT_DELEGATE_H_

#include <stdint.h>

#include "base/macros.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/message_center/views/popup_alignment_delegate.h"

namespace display {
class Screen;
}

namespace message_center {
namespace test {
class MessagePopupCollectionTest;
}

// The PopupAlignmentDelegate for non-ash Windows/Linux desktop.
class MESSAGE_CENTER_EXPORT DesktopPopupAlignmentDelegate
    : public PopupAlignmentDelegate,
      public display::DisplayObserver {
 public:
  DesktopPopupAlignmentDelegate();
  ~DesktopPopupAlignmentDelegate() override;

  void StartObserving(display::Screen* screen);

  // Overridden from PopupAlignmentDelegate:
  int GetToastOriginX(const gfx::Rect& toast_bounds) const override;
  int GetBaseline() const override;
  gfx::Rect GetWorkArea() const override;
  bool IsTopDown() const override;
  bool IsFromLeft() const override;
  void RecomputeAlignment(const display::Display& display) override;
  void ConfigureWidgetInitParamsForContainer(
      views::Widget* widget,
      views::Widget::InitParams* init_params) override;
  bool IsPrimaryDisplayForNotification() const override;

 private:
  friend class test::MessagePopupCollectionTest;

  enum PopupAlignment {
    POPUP_ALIGNMENT_TOP = 1 << 0,
    POPUP_ALIGNMENT_LEFT = 1 << 1,
    POPUP_ALIGNMENT_BOTTOM = 1 << 2,
    POPUP_ALIGNMENT_RIGHT = 1 << 3,
  };

  void UpdatePrimaryDisplay();

  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  int32_t alignment_;
  int64_t primary_display_id_;
  display::Screen* screen_;
  gfx::Rect work_area_;

  DISALLOW_COPY_AND_ASSIGN(DesktopPopupAlignmentDelegate);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_DESKTOP_POPUP_ALIGNMENT_DELEGATE_H_
