// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_filter/false_touch_finder.h"

#include <stddef.h>

#include <algorithm>
#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/ozone/evdev/touch_evdev_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace ui {

class FalseTouchFinderTest : public testing::Test {
 public:
  struct TouchEntry {
    int time_ms;
    size_t slot;
    bool touching;
    gfx::PointF location;
    bool expect_noise;
    bool expect_delay;
  };

  FalseTouchFinderTest() {}
  ~FalseTouchFinderTest() override {}

  bool FilterAndCheck(const TouchEntry entries[], size_t count) {
    std::vector<InProgressTouchEvdev> touches;
    size_t start_index = 0u;
    std::bitset<kNumTouchEvdevSlots> was_touching;
    for (size_t i = 0; i < count; ++i) {
      const TouchEntry& entry = entries[i];

      InProgressTouchEvdev touch;
      touch.x = entry.location.x();
      touch.y = entry.location.y();
      touch.tracking_id = entry.slot;
      touch.slot = entry.slot;
      touch.was_touching = was_touching.test(touch.slot);
      touch.touching = entry.touching;
      touches.push_back(touch);

      if (i == count - 1 || entry.time_ms != entries[i + 1].time_ms) {
        false_touch_finder_->HandleTouches(touches, base::TimeTicks() +
            base::TimeDelta::FromMilliseconds(entry.time_ms));

        for (size_t j = 0; j < touches.size(); ++j) {
          bool expect_noise = entries[j + start_index].expect_noise;
          bool expect_delay = entries[j + start_index].expect_delay;
          size_t slot = touches[j].slot;
          if (false_touch_finder_->SlotHasNoise(slot) != expect_noise
              || false_touch_finder_->SlotShouldDelay(slot) != expect_delay) {
            LOG(ERROR) << base::StringPrintf(
                "Incorrect filtering at %dms for slot %zu", entry.time_ms,
                slot);
            return false;
          }
        }

        start_index = i + 1;
        touches.clear();
      }

      was_touching.set(entry.slot, entry.touching);
    }

    return true;
  }

 gfx::Size touchscreen_size = gfx::Size(4000,4000);

 private:
  // testing::Test:
  void SetUp() override {
    false_touch_finder_.reset(
        new FalseTouchFinder(true, true, touchscreen_size));
  }

  std::unique_ptr<FalseTouchFinder> false_touch_finder_;

  DISALLOW_COPY_AND_ASSIGN(FalseTouchFinderTest);
};

// Test that taps which are far apart in quick succession are considered noise.
TEST_F(FalseTouchFinderTest, FarApartTaps) {
  const TouchEntry kTestData[] = {
      {10, 1, true, gfx::PointF(10, 10), false, false},
      {20, 1, true, gfx::PointF(10, 11), false, false},
      {30, 1, true, gfx::PointF(10, 12), false, false},
      {30, 2, true, gfx::PointF(2500, 1000), true, false},
      {40, 1, true, gfx::PointF(10, 13), true, false},
      {40, 2, true, gfx::PointF(2500, 1001), true, false},
      {50, 1, true, gfx::PointF(10, 14), true, false},
      {50, 2, false, gfx::PointF(2500, 1002), true, false},
      {60, 1, false, gfx::PointF(10, 15), true, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that taps which are far apart but do not occur in quick succession are
// not considered noise.
TEST_F(FalseTouchFinderTest, FarApartTapsSlow) {
  const TouchEntry kTestData[] = {
      {1000, 1, true, gfx::PointF(10, 10), false, false},
      {1500, 1, true, gfx::PointF(10, 11), false, false},
      {2000, 1, true, gfx::PointF(10, 12), false, false},
      {2500, 1, true, gfx::PointF(10, 13), false, false},
      {2500, 2, true, gfx::PointF(2500, 1000), false, false},
      {3000, 1, true, gfx::PointF(10, 14), false, false},
      {3000, 2, false, gfx::PointF(2500, 1001), false, false},
      {3500, 1, false, gfx::PointF(10, 15), false, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that touches which are horizontally aligned are considered noise.
TEST_F(FalseTouchFinderTest, HorizontallyAligned) {
  const TouchEntry kTestData[] = {
      {10, 1, true, gfx::PointF(10, 10), false, false},
      {20, 1, true, gfx::PointF(10, 10), false, false},
      {20, 2, true, gfx::PointF(10, 25), true, false},
      {30, 1, false, gfx::PointF(10, 10), false, false},
      {30, 2, true, gfx::PointF(10, 25), true, false},
      {40, 2, false, gfx::PointF(10, 25), true, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that touches in the same position are considered noise.
TEST_F(FalseTouchFinderTest, SamePosition) {
  const TouchEntry kTestData[] = {
      {1000, 1, true, gfx::PointF(10, 10), false, false},
      {1500, 1, false, gfx::PointF(10, 10), false, false},
      {2000, 1, true, gfx::PointF(10, 10), false, false},
      {2500, 1, false, gfx::PointF(10, 10), false, false},
      {3000, 1, true, gfx::PointF(50, 50), false, false},
      {3500, 1, true, gfx::PointF(50, 51), false, false},
      {3500, 2, true, gfx::PointF(10, 10), true, false},
      {4000, 1, false, gfx::PointF(50, 52), false, false},
      {4000, 2, false, gfx::PointF(10, 10), true, false},
      {4500, 1, true, gfx::PointF(10, 10), true, false},
      {5000, 1, false, gfx::PointF(10, 10), true, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that a multi-second touch is considered noise.
TEST_F(FalseTouchFinderTest, MultiSecondTouch) {
  const TouchEntry kTestData[] = {
      {1000, 1, true, gfx::PointF(10, 10), false, false},
      {2000, 1, true, gfx::PointF(10, 11), false, false},
      {3000, 1, true, gfx::PointF(10, 10), false, false},
      {4000, 1, true, gfx::PointF(10, 11), true, false},
      {5000, 1, true, gfx::PointF(10, 10), true, false},
      {6000, 1, true, gfx::PointF(10, 11), true, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that a touch on the edge which never leaves is delayed and never
// released.
TEST_F(FalseTouchFinderTest, EdgeTap) {
  int ts_width = touchscreen_size.width();
  int ts_height = touchscreen_size.height();
  const TouchEntry kTestData[] = {
      {10, 1, true, gfx::PointF(0, 100), false, true},
      {20, 1, true, gfx::PointF(0, 100), false, true},
      {30, 1, false, gfx::PointF(0, 100), false, true},
      {40, 2, true, gfx::PointF(ts_width - 1, 100), false, true},
      {50, 2, true, gfx::PointF(ts_width - 1, 100), false, true},
      {60, 2, false, gfx::PointF(ts_width - 1, 100), false, true},
      {70, 3, true, gfx::PointF(100, 0), false, true},
      {80, 3, true, gfx::PointF(100, 0), false, true},
      {90, 3, false, gfx::PointF(100, 0), false, true},
      {100, 4, true, gfx::PointF(100, ts_height - 1), false, true},
      {110, 4, true, gfx::PointF(100, ts_height - 1), false, true},
      {120, 4, false, gfx::PointF(100, ts_height - 1), false, true}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that a touch on the edge which starts at an edge is delayed but released
// as soon as it moves.
TEST_F(FalseTouchFinderTest, MoveFromEdge) {
  const TouchEntry kTestData[] = {
      {10, 1, true, gfx::PointF(0, 100), false, true},
      {20, 1, true, gfx::PointF(0, 100), false, true},
      {30, 1, true, gfx::PointF(1, 100), false, false},
      {40, 1, false, gfx::PointF(1, 100), false, false},
      {50, 1, true, gfx::PointF(0, 100), false, true},
      {60, 1, true, gfx::PointF(0, 100), false, true},
      {70, 1, true, gfx::PointF(0, 101), false, false},
      {80, 1, false, gfx::PointF(0, 101), false, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

// Test that a touch on the edge which starts away from the edge is not
// cancelled when it moves to the edge.
TEST_F(FalseTouchFinderTest, MoveToEdge) {
  const TouchEntry kTestData[] = {
      {10, 1, true, gfx::PointF(100, 100), false, false},
      {20, 1, true, gfx::PointF(100, 100), false, false},
      {30, 1, true, gfx::PointF(0, 100), false, false},
      {40, 1, false, gfx::PointF(0, 100), false, false}};
  EXPECT_TRUE(FilterAndCheck(kTestData, arraysize(kTestData)));
}

}  // namespace ui
