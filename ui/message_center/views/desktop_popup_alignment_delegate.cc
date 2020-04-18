// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/desktop_popup_alignment_delegate.h"

#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/message_center/public/cpp/message_center_constants.h"

namespace message_center {

DesktopPopupAlignmentDelegate::DesktopPopupAlignmentDelegate()
    : alignment_(POPUP_ALIGNMENT_BOTTOM | POPUP_ALIGNMENT_RIGHT),
      primary_display_id_(display::kInvalidDisplayId),
      screen_(NULL) {}

DesktopPopupAlignmentDelegate::~DesktopPopupAlignmentDelegate() {
  if (screen_)
    screen_->RemoveObserver(this);
}

void DesktopPopupAlignmentDelegate::StartObserving(display::Screen* screen) {
  if (screen_ || !screen)
    return;

  screen_ = screen;
  screen_->AddObserver(this);
  display::Display display = screen_->GetPrimaryDisplay();
  primary_display_id_ = display.id();
  RecomputeAlignment(display);
}

int DesktopPopupAlignmentDelegate::GetToastOriginX(
    const gfx::Rect& toast_bounds) const {
  if (IsFromLeft())
    return work_area_.x() + kMarginBetweenPopups;
  return work_area_.right() - kMarginBetweenPopups - toast_bounds.width();
}

int DesktopPopupAlignmentDelegate::GetBaseline() const {
  return IsTopDown() ? work_area_.y() + kMarginBetweenPopups
                     : work_area_.bottom() - kMarginBetweenPopups;
}

gfx::Rect DesktopPopupAlignmentDelegate::GetWorkArea() const {
  return work_area_;
}

bool DesktopPopupAlignmentDelegate::IsTopDown() const {
  return (alignment_ & POPUP_ALIGNMENT_TOP) != 0;
}

bool DesktopPopupAlignmentDelegate::IsFromLeft() const {
  return (alignment_ & POPUP_ALIGNMENT_LEFT) != 0;
}

void DesktopPopupAlignmentDelegate::RecomputeAlignment(
    const display::Display& display) {
  if (work_area_ == display.work_area())
    return;

  work_area_ = display.work_area();

  // If the taskbar is at the top, render notifications top down. Some platforms
  // like Gnome can have taskbars at top and bottom. In this case it's more
  // likely that the systray is on the top one.
  alignment_ = work_area_.y() > display.bounds().y() ? POPUP_ALIGNMENT_TOP
                                                     : POPUP_ALIGNMENT_BOTTOM;

  // If the taskbar is on the left show the notifications on the left. Otherwise
  // show it on right since it's very likely that the systray is on the right if
  // the taskbar is on the top or bottom.
  // Since on some platforms like Ubuntu Unity there's also a launcher along
  // with a taskbar (panel), we need to check that there is really nothing at
  // the top before concluding that the taskbar is at the left.
  alignment_ |= (work_area_.x() > display.bounds().x() &&
                 work_area_.y() == display.bounds().y())
      ? POPUP_ALIGNMENT_LEFT
      : POPUP_ALIGNMENT_RIGHT;
}

void DesktopPopupAlignmentDelegate::ConfigureWidgetInitParamsForContainer(
    views::Widget* widget,
    views::Widget::InitParams* init_params) {
  // Do nothing, which will use the default container.
}

bool DesktopPopupAlignmentDelegate::IsPrimaryDisplayForNotification() const {
  return true;
}

// Anytime the display configuration changes, we need to recompute the alignment
// on the primary display. But, we get different events on different platforms.
// On Windows, for example, when switching from a laptop display to an external
// monitor, we get a OnDisplayMetricsChanged() event. On Linux, we get a
// OnDisplayRemoved() and a OnDisplayAdded() instead. In order to account for
// these slightly different abstractions, we update on every event.
void DesktopPopupAlignmentDelegate::UpdatePrimaryDisplay() {
  display::Display primary_display = screen_->GetPrimaryDisplay();
  if (primary_display.id() != primary_display_id_) {
    primary_display_id_ = primary_display.id();
    RecomputeAlignment(primary_display);
    DoUpdateIfPossible();
  }
}

void DesktopPopupAlignmentDelegate::OnDisplayAdded(
    const display::Display& added_display) {
  // The added display could be the new primary display.
  UpdatePrimaryDisplay();
}

void DesktopPopupAlignmentDelegate::OnDisplayRemoved(
    const display::Display& removed_display) {
  // The removed display may have been the primary display.
  UpdatePrimaryDisplay();
}

void DesktopPopupAlignmentDelegate::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t metrics) {
  // Set to kInvalidDisplayId so the alignment is updated regardless of whether
  // the primary display actually changed.
  primary_display_id_ = display::kInvalidDisplayId;
  UpdatePrimaryDisplay();
}

}  // namespace message_center
