// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/material_design_controller_test_api.h"

namespace ui {
namespace test {

MaterialDesignControllerTestAPI::MaterialDesignControllerTestAPI(
    MaterialDesignController::Mode mode)
    : previous_mode_(MaterialDesignController::mode_),
      previous_initialized_(MaterialDesignController::is_mode_initialized_) {
  MaterialDesignController::SetMode(mode);
}

MaterialDesignControllerTestAPI::~MaterialDesignControllerTestAPI() {
  MaterialDesignController::is_mode_initialized_ = previous_initialized_;
  MaterialDesignController::mode_ = previous_mode_;
}

void MaterialDesignControllerTestAPI::Uninitialize() {
  MaterialDesignController::Uninitialize();
}

}  // namespace test
}  // namespace ui
