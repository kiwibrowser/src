// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/memory_pressure_controller_impl.h"

#include "base/logging.h"

namespace chromecast {

MemoryPressureControllerImpl::MemoryPressureControllerImpl() {
  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::BindRepeating(&MemoryPressureControllerImpl::OnMemoryPressure,
                          base::Unretained(this))));
}

MemoryPressureControllerImpl::~MemoryPressureControllerImpl() = default;

void MemoryPressureControllerImpl::AddBinding(
    mojom::MemoryPressureControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MemoryPressureControllerImpl::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel level) {
  observers_.ForAllPtrs([level](mojom::MemoryPressureObserver* observer) {
    observer->MemoryPressureLevelChanged(level);
  });
}

void MemoryPressureControllerImpl::AddObserver(
    mojom::MemoryPressureObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

}  // namespace chromecast
