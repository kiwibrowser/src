// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/laser/laser_pointer_controller.h"

#include <memory>

#include "ash/laser/laser_pointer_view.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/system/palette/palette_utils.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// A point gets removed from the collection if it is older than
// |kPointLifeDurationMs|.
const int kPointLifeDurationMs = 200;

// When no move events are being received we add a new point every
// |kAddStationaryPointsDelayMs| so that points older than
// |kPointLifeDurationMs| can get removed.
// Note: Using a delay less than the screen refresh interval will not
// provide a visual benefit but instead just waste time performing
// unnecessary updates. 16ms is the refresh interval on most devices.
// TODO(reveman): Use real VSYNC interval instead of a hard-coded delay.
const int kAddStationaryPointsDelayMs = 16;

}  // namespace

LaserPointerController::LaserPointerController() {
  Shell::Get()->AddPreTargetHandler(this);
}

LaserPointerController::~LaserPointerController() {
  Shell::Get()->RemovePreTargetHandler(this);
}

void LaserPointerController::SetEnabled(bool enabled) {
  FastInkPointerController::SetEnabled(enabled);
  if (!enabled)
    DestroyPointerView();
}

views::View* LaserPointerController::GetPointerView() const {
  return laser_pointer_view_.get();
}

void LaserPointerController::CreatePointerView(
    base::TimeDelta presentation_delay,
    aura::Window* root_window) {
  laser_pointer_view_ = std::make_unique<LaserPointerView>(
      base::TimeDelta::FromMilliseconds(kPointLifeDurationMs),
      presentation_delay,
      base::TimeDelta::FromMilliseconds(kAddStationaryPointsDelayMs),
      Shell::GetContainer(root_window, kShellWindowId_OverlayContainer));
}

void LaserPointerController::UpdatePointerView(ui::TouchEvent* event) {
  laser_pointer_view_->AddNewPoint(event->root_location_f(),
                                   event->time_stamp());
  if (event->type() == ui::ET_TOUCH_RELEASED) {
    laser_pointer_view_->FadeOut(base::BindOnce(
        &LaserPointerController::DestroyPointerView, base::Unretained(this)));
  }
}

void LaserPointerController::DestroyPointerView() {
  laser_pointer_view_.reset();
}

bool LaserPointerController::CanStartNewGesture(ui::TouchEvent* event) {
  // Ignore events over the palette.
  if (ash::palette_utils::PaletteContainsPointInScreen(event->root_location()))
    return false;
  return FastInkPointerController::CanStartNewGesture(event);
}

}  // namespace ash
