// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FALSE_TOUCH_FINDER_H_
#define UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FALSE_TOUCH_FINDER_H_

#include <stddef.h>

#include <bitset>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/events/event_utils.h"
#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/events/ozone/evdev/touch_evdev_types.h"

namespace ui {

class TouchFilter;

// Finds touches which are should be filtered.
class EVENTS_OZONE_EVDEV_EXPORT FalseTouchFinder {
 public:
  FalseTouchFinder(bool touch_noise_filtering,
              bool edge_filtering,
              gfx::Size touchscreen_size);
  ~FalseTouchFinder();

  // Updates which ABS_MT_SLOTs should be filtered. |touches| should contain
  // all of the in-progress touches at |time| (including filtered touches).
  // |touches| should have at most one entry per ABS_MT_SLOT.
  void HandleTouches(const std::vector<InProgressTouchEvdev>& touches,
                     base::TimeTicks time);

  // Returns whether the in-progress touch at ABS_MT_SLOT |slot| has noise.
  // These slots should be cancelled
  bool SlotHasNoise(size_t slot) const;

  // Returns whether the in-progress touch at ABS_MT_SLOT |slot| should delay
  // reporting. They may be later reported.
  bool SlotShouldDelay(size_t slot) const;

 private:
  // Records how frequently noisy touches occur to UMA.
  void RecordUMA(bool had_noise, base::TimeTicks time);

  friend class TouchEventConverterEvdevTouchNoiseTest;

  // The slots which are noise.
  std::bitset<kNumTouchEvdevSlots> slots_with_noise_;

  // The slots which should delay.
  std::bitset<kNumTouchEvdevSlots> slots_should_delay_;

  // The time of the previous noise occurrence in any of the slots.
  base::TimeTicks last_noise_time_;

  // Noise filters detect noise. Any slot with detected noise should be
  // cancelled by the touch converter.
  std::vector<std::unique_ptr<TouchFilter>> noise_filters_;

  // The edge filter detects taps on the edge. The edge filter should only
  // filter slots which it is already filtering or which are new.
  std::unique_ptr<TouchFilter> edge_touch_filter_;

  DISALLOW_COPY_AND_ASSIGN(FalseTouchFinder);
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_FALSE_TOUCH_FINDER_H_
