// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/laser/laser_pointer_controller_test_api.h"

#include "ash/components/fast_ink/fast_ink_points.h"
#include "ash/laser/laser_pointer_controller.h"
#include "ash/laser/laser_pointer_view.h"

namespace ash {

LaserPointerControllerTestApi::LaserPointerControllerTestApi(
    LaserPointerController* instance)
    : instance_(instance) {}

LaserPointerControllerTestApi::~LaserPointerControllerTestApi() = default;

void LaserPointerControllerTestApi::SetEnabled(bool enabled) {
  instance_->SetEnabled(enabled);
}

bool LaserPointerControllerTestApi::IsShowingLaserPointer() const {
  return instance_->laser_pointer_view_ != nullptr;
}

bool LaserPointerControllerTestApi::IsFadingAway() const {
  return IsShowingLaserPointer() &&
         !instance_->laser_pointer_view_->fadeout_done_.is_null();
}

const fast_ink::FastInkPoints& LaserPointerControllerTestApi::laser_points()
    const {
  return instance_->laser_pointer_view_->laser_points_;
}

const fast_ink::FastInkPoints&
LaserPointerControllerTestApi::predicted_laser_points() const {
  return instance_->laser_pointer_view_->predicted_laser_points_;
}

LaserPointerView* LaserPointerControllerTestApi::laser_pointer_view() const {
  return instance_->laser_pointer_view_.get();
}

}  // namespace ash
