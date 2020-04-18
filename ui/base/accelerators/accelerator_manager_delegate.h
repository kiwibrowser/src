// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_DELEGATE_H_
#define UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_DELEGATE_H_

#include "ui/base/ui_base_export.h"

namespace ui {

class Accelerator;

class UI_BASE_EXPORT AcceleratorManagerDelegate {
 public:
  // Called when new accelerators are registered. This is only called with
  // newly registered accelerators. For example, if Register() is
  // called with A and B, then OnAcceleratorsRegistered() is called with A and
  // B. If Register() is subsequently called with A and C, then
  // OnAcceleratorsRegistered() is only called with C, as A was already
  // registered.
  virtual void OnAcceleratorsRegistered(
      const std::vector<ui::Accelerator>& accelerators) = 0;

  // Called when there no more targets are registered for |accelerator|.
  virtual void OnAcceleratorUnregistered(const Accelerator& accelerator) = 0;

 protected:
  virtual ~AcceleratorManagerDelegate() {}
};

}  // namespace ui

#endif  // UI_BASE_ACCELERATORS_ACCELERATOR_MANAGER_DELEGATE_H_
