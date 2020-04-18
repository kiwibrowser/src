// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/two_step_edge_cycler.h"

#include <cstdlib>

namespace ash {
namespace {

// We cycle to the second mode if any of the following happens while the mouse
// is on the edge of the workspace:
// . The user stops moving the mouse for |kMaxDelay| and then moves the mouse
//   again in the preferred direction from the last paused location for at least
//   |kMaxPixelsAfterPause| horizontal pixels.
// . The mouse moves |kMaxPixels| horizontal pixels in the preferred direction.
// . The mouse is moved |kMaxMoves| times since the last pause.
const int kMaxDelay = 400;
const int kMaxPixels = 100;
const int kMaxPixelsAfterPause = 10;
const int kMaxMoves = 25;

}  // namespace

TwoStepEdgeCycler::TwoStepEdgeCycler(const gfx::Point& start,
                                     TwoStepEdgeCycler::Direction direction)
    : second_mode_(false),
      time_last_move_(base::TimeTicks::Now()),
      num_moves_(0),
      start_x_(start.x()),
      paused_x_(start.x()),
      paused_(false),
      direction_(direction) {}

TwoStepEdgeCycler::~TwoStepEdgeCycler() = default;

void TwoStepEdgeCycler::OnMove(const gfx::Point& location) {
  if (second_mode_)
    return;

  if ((base::TimeTicks::Now() - time_last_move_).InMilliseconds() > kMaxDelay) {
    paused_ = true;
    paused_x_ = location.x();
    num_moves_ = 0;
  }
  time_last_move_ = base::TimeTicks::Now();

  int compare_x = paused_ ? paused_x_ : start_x_;
  if (location.x() != compare_x &&
      (location.x() < compare_x) != (direction_ == DIRECTION_LEFT)) {
    return;
  }

  ++num_moves_;
  bool moved_in_the_same_direction_after_pause =
      paused_ && std::abs(location.x() - paused_x_) >= kMaxPixelsAfterPause;
  second_mode_ = moved_in_the_same_direction_after_pause ||
                 std::abs(location.x() - start_x_) >= kMaxPixels ||
                 num_moves_ >= kMaxMoves;
}

}  // namespace ash
