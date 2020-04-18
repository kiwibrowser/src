// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ime/ime_registrar_impl.h"

namespace ui {

IMERegistrarImpl::IMERegistrarImpl(IMEDriverBridge* ime_driver_bridge)
    : ime_driver_bridge_(ime_driver_bridge) {}

IMERegistrarImpl::~IMERegistrarImpl() {}

void IMERegistrarImpl::AddBinding(mojom::IMERegistrarRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void IMERegistrarImpl::RegisterDriver(mojom::IMEDriverPtr driver) {
  // TODO(moshayedi): crbug.com/634441. IMERegistrarImpl currently identifies
  // the last registered driver as the current driver. Rethink this once we
  // have more usecases.
  ime_driver_bridge_->SetDriver(std::move(driver));
}
}  // namespace ui
