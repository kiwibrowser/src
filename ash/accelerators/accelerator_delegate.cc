// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_delegate.h"

#include "ash/accelerators/accelerator_router.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {

AcceleratorDelegate::AcceleratorDelegate() : router_(new AcceleratorRouter) {}

AcceleratorDelegate::~AcceleratorDelegate() = default;

bool AcceleratorDelegate::ProcessAccelerator(
    const ui::KeyEvent& key_event,
    const ui::Accelerator& accelerator) {
  return router_->ProcessAccelerator(
      static_cast<aura::Window*>(key_event.target()), key_event, accelerator);
}

}  // namespace ash
