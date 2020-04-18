// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_MEMORY_PRESSURE_CONTROLLER_IMPL_H_
#define CHROMECAST_BROWSER_MEMORY_PRESSURE_CONTROLLER_IMPL_H_

#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "chromecast/common/mojom/memory_pressure.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace chromecast {

class MemoryPressureControllerImpl : public mojom::MemoryPressureController {
 public:
  MemoryPressureControllerImpl();
  ~MemoryPressureControllerImpl() override;

  void AddBinding(mojom::MemoryPressureControllerRequest request);

 private:
  // chromecast::mojom::MemoryPressure implementation.
  void AddObserver(mojom::MemoryPressureObserverPtr observer) override;

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel level);

  mojo::InterfacePtrSet<mojom::MemoryPressureObserver> observers_;
  mojo::BindingSet<mojom::MemoryPressureController> bindings_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  DISALLOW_COPY_AND_ASSIGN(MemoryPressureControllerImpl);
};

}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_MEMORY_PRESSURE_CONTROLLER_IMPL_H_
