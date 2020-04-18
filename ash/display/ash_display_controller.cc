// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/ash_display_controller.h"

#include "ash/shell.h"
#include "ui/display/manager/display_configurator.h"

namespace ash {

AshDisplayController::AshDisplayController() = default;

AshDisplayController::~AshDisplayController() = default;

void AshDisplayController::BindRequest(
    mojom::AshDisplayControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void AshDisplayController::TakeDisplayControl(
    TakeDisplayControlCallback callback) {
  Shell::Get()->display_configurator()->TakeControl(std::move(callback));
}

void AshDisplayController::RelinquishDisplayControl(
    RelinquishDisplayControlCallback callback) {
  Shell::Get()->display_configurator()->RelinquishControl(std::move(callback));
}

}  // namespace ash
