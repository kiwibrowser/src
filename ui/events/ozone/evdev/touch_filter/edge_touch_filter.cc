// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_filter/edge_touch_filter.h"

#include <stddef.h>

#include <cmath>

#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "ui/gfx/geometry/insets.h"

namespace ui {


namespace {

// The maximum distance from the border to be considered for filtering
const int kMaxBorderDistance = 1;

bool IsNearBorder(const gfx::Point& point, gfx::Size touchscreen_size) {
  gfx::Rect inner_bound = gfx::Rect(touchscreen_size);
  inner_bound.Inset(gfx::Insets(kMaxBorderDistance));
  return !inner_bound.Contains(point);
}

}  // namespace

EdgeTouchFilter::EdgeTouchFilter(gfx::Size& touchscreen_size)
    : touchscreen_size_(touchscreen_size) {
}

void EdgeTouchFilter::Filter(
    const std::vector<InProgressTouchEvdev>& touches,
    base::TimeTicks time,
    std::bitset<kNumTouchEvdevSlots>* slots_should_delay) {

  for (const InProgressTouchEvdev& touch : touches) {
    size_t slot = touch.slot;
    gfx::Point& tracked_tap = tracked_taps_[slot];
    gfx::Point touch_pos = gfx::Point(touch.x, touch.y);
    bool should_delay = false;

    if (!touch.touching && !touch.was_touching)
      continue;  // Only look at slots with active touches.

    if (!touch.was_touching && IsNearBorder(touch_pos, touchscreen_size_)) {
      should_delay = true;  // Delay new contact near border.
      tracked_taps_[slot] = touch_pos;
    }

    if (slots_should_delay->test(slot) && touch_pos == tracked_tap) {
      should_delay = true;  // Continue delaying contacts that don't move.
    }

    slots_should_delay->set(slot, should_delay);
  }
}

}  // namespace ui
