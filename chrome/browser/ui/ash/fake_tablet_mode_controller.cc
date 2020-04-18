// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/fake_tablet_mode_controller.h"

FakeTabletModeController::FakeTabletModeController() : binding_(this) {}

FakeTabletModeController::~FakeTabletModeController() = default;

ash::mojom::TabletModeControllerPtr
FakeTabletModeController::CreateInterfacePtr() {
  ash::mojom::TabletModeControllerPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void FakeTabletModeController::SetClient(
    ash::mojom::TabletModeClientPtr client) {
  was_client_set_ = true;
}
