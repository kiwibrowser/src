// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_ACCELERATOR_DELEGATE_H_
#define ASH_ACCELERATORS_ACCELERATOR_DELEGATE_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/wm/core/accelerator_delegate.h"

namespace ash {

class AcceleratorRouter;

class ASH_EXPORT AcceleratorDelegate : public ::wm::AcceleratorDelegate {
 public:
  AcceleratorDelegate();
  ~AcceleratorDelegate() override;

  // wm::AcceleratorDelegate:
  bool ProcessAccelerator(const ui::KeyEvent& event,
                          const ui::Accelerator& accelerator) override;

 private:
  std::unique_ptr<AcceleratorRouter> router_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorDelegate);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_ACCELERATOR_DELEGATE_H_
