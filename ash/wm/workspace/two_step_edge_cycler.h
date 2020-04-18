// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_TWO_STEP_EDGE_CYCLER_H_
#define ASH_WM_WORKSPACE_TWO_STEP_EDGE_CYCLER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/point.h"

namespace ash {

// TwoStepEdgeCycler is responsible for cycling between two modes when the mouse
// is at the edge of the workspace. The cycler does not loop so it is impossible
// to get back to the first mode once the second mode is reached.
// TwoStepEdgeCycler should be destroyed once the mouse moves off the edge of
// the workspace.
class ASH_EXPORT TwoStepEdgeCycler {
 public:
  // The direction in which a mouse should travel to switch mode.
  enum Direction { DIRECTION_LEFT, DIRECTION_RIGHT };

  explicit TwoStepEdgeCycler(const gfx::Point& start, Direction direction);
  ~TwoStepEdgeCycler();

  // Update which mode should be used as a result of a mouse / touch move.
  // |location| is the location of the event.
  void OnMove(const gfx::Point& location);

  bool use_second_mode() const { return second_mode_; }

 private:
  // Whether the second mode should be used.
  bool second_mode_;

  // Time OnMove() was last invoked.
  base::TimeTicks time_last_move_;

  // The number of moves since the cycler was constructed.
  int num_moves_;

  // Initial x-coordinate.
  int start_x_;

  // x-coordinate when paused.
  int paused_x_;

  // Whether the movement was paused.
  bool paused_;

  // Determines a preferred movement direction that we are watching.
  Direction direction_;

  DISALLOW_COPY_AND_ASSIGN(TwoStepEdgeCycler);
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_TWO_STEP_EDGE_CYCLER_H_
