// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/screen_manager_ozone_external.h"

#include <memory>

#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_constants.h"

namespace display {

// static
std::unique_ptr<ScreenManager> ScreenManager::Create() {
  return std::make_unique<ScreenManagerOzoneExternal>();
}

ScreenManagerOzoneExternal::ScreenManagerOzoneExternal()
    : screen_(std::make_unique<display::ScreenBase>()) {}

ScreenManagerOzoneExternal::~ScreenManagerOzoneExternal() {}

void ScreenManagerOzoneExternal::AddInterfaces(
    service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>* registry) {}

void ScreenManagerOzoneExternal::Init(ScreenManagerDelegate* delegate) {}

void ScreenManagerOzoneExternal::RequestCloseDisplay(int64_t display_id) {}

display::ScreenBase* ScreenManagerOzoneExternal::GetScreen() {
  return screen_.get();
}

}  // namespace display
