// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_ACCELERATOR_ROUTER_H_
#define ASH_ACCELERATORS_ACCELERATOR_ROUTER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/time.h"

namespace aura {
class Window;
}

namespace ui {
class Accelerator;
class KeyEvent;
}

namespace ash {

// AcceleratorRouter does a minimal amount of processing before routing the
// accelerator to the AcceleratorController. AcceleratorRouter may also decide
// not to process certain accelerators.
class ASH_EXPORT AcceleratorRouter {
 public:
  AcceleratorRouter();
  ~AcceleratorRouter();

  // Returns true if event should be consumed. |target| is the target of the
  // event.
  bool ProcessAccelerator(aura::Window* target,
                          const ui::KeyEvent& event,
                          const ui::Accelerator& accelerator);

 private:
  // Returns true if the window should be allowed a chance to handle
  // system keys.
  bool CanConsumeSystemKeys(aura::Window* target, const ui::KeyEvent& event);

  // Returns true if the |accelerator| should be processed now.
  bool ShouldProcessAcceleratorNow(aura::Window* target,
                                   const ui::KeyEvent& event,
                                   const ui::Accelerator& accelerator);

  // Records a histogram on how long the "Search" key is held when a user
  // presses an accelerator that involes the "Search" key.
  void RecordSearchKeyStats(const ui::Accelerator& accelerator);

  enum SearchKeyState { RELEASED = 0, PRESSED, RECORDED };
  SearchKeyState search_key_state_ = RELEASED;
  base::TimeTicks search_key_pressed_timestamp_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorRouter);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_ACCELERATOR_ROUTER_H_
