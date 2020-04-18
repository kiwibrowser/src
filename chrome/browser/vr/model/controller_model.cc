// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/controller_model.h"

namespace vr {

ControllerModel::ControllerModel() = default;

ControllerModel::ControllerModel(const ControllerModel& other)
    : transform(other.transform),
      laser_direction(other.laser_direction),
      laser_origin(other.laser_origin),
      touchpad_button_state(other.touchpad_button_state),
      app_button_state(other.app_button_state),
      home_button_state(other.home_button_state),
      opacity(other.opacity),
      quiescent(other.quiescent),
      resting_in_viewport(other.resting_in_viewport),
      handedness(other.handedness) {}

ControllerModel::~ControllerModel() = default;

}  // namespace vr
