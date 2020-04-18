// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FAR_APART_TAPS_TOUCH_NOISE_FILTER_H_
#define UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FAR_APART_TAPS_TOUCH_NOISE_FILTER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/events/ozone/evdev/touch_filter/touch_filter.h"

namespace ui {

class FarApartTapsTouchNoiseFilter : public TouchFilter {
 public:
  FarApartTapsTouchNoiseFilter() {}
  ~FarApartTapsTouchNoiseFilter() override {}

  // TouchFilter:
  void Filter(const std::vector<InProgressTouchEvdev>& touches,
              base::TimeTicks time,
              std::bitset<kNumTouchEvdevSlots>* slots_with_noise) override;

 private:
  struct Tap {
    Tap() : x(0), y(0) {}
    Tap(base::TimeTicks start, int x, int y)
        : start(start), x(x), y(y) {}

    bool is_valid() const { return !start.is_null(); }
    void Invalidate() { start = base::TimeTicks(); }

    base::TimeTicks start;
    float x;
    float y;
  };

  // Tracks in progress taps in slots.
  Tap tracked_taps_[kNumTouchEvdevSlots];

  DISALLOW_COPY_AND_ASSIGN(FarApartTapsTouchNoiseFilter);
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FAR_APART_TAPS_TOUCH_NOISE_FILTER_H_
