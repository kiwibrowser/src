// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LASER_LASER_POINTER_CONTROLLER_TEST_API_H_
#define ASH_LASER_LASER_POINTER_CONTROLLER_TEST_API_H_

#include "base/macros.h"

namespace fast_ink {
class FastInkPoints;
}

namespace ash {

class LaserPointerController;
class LaserPointerView;

// An api for testing the LaserPointerController class.
class LaserPointerControllerTestApi {
 public:
  explicit LaserPointerControllerTestApi(LaserPointerController* instance);
  ~LaserPointerControllerTestApi();

  void SetEnabled(bool enabled);
  bool IsShowingLaserPointer() const;
  bool IsFadingAway() const;
  const fast_ink::FastInkPoints& laser_points() const;
  const fast_ink::FastInkPoints& predicted_laser_points() const;
  LaserPointerView* laser_pointer_view() const;

 private:
  LaserPointerController* instance_;

  DISALLOW_COPY_AND_ASSIGN(LaserPointerControllerTestApi);
};

}  // namespace ash

#endif  // ASH_LASER_LASER_POINTER_CONTROLLER_TEST_API_H_
