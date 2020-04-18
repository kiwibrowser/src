// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LASER_LASER_POINTER_CONTROLLER_H_
#define ASH_LASER_LASER_POINTER_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/components/fast_ink/fast_ink_pointer_controller.h"

namespace ash {

class LaserPointerView;

// Controller for the laser pointer functionality. Enables/disables laser
// pointer as well as receives points and passes them off to be rendered.
class ASH_EXPORT LaserPointerController
    : public fast_ink::FastInkPointerController {
 public:
  LaserPointerController();
  ~LaserPointerController() override;

  // fast_ink::FastInkPointerController:
  void SetEnabled(bool enabled) override;

 private:
  friend class LaserPointerControllerTestApi;

  // fast_ink::FastInkPointerController:
  views::View* GetPointerView() const override;
  void CreatePointerView(base::TimeDelta presentation_delay,
                         aura::Window* root_window) override;
  void UpdatePointerView(ui::TouchEvent* event) override;
  void DestroyPointerView() override;
  bool CanStartNewGesture(ui::TouchEvent* event) override;

  // |laser_pointer_view_| will only hold an instance when the laser pointer is
  // enabled and activated (pressed or dragged).
  std::unique_ptr<LaserPointerView> laser_pointer_view_;

  DISALLOW_COPY_AND_ASSIGN(LaserPointerController);
};

}  // namespace ash

#endif  // ASH_LASER_LASER_POINTER_CONTROLLER_H_
