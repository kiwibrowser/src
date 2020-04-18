// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ASH_METRICS_POINTER_METRICS_RECORDER_H_
#define UI_ASH_METRICS_POINTER_METRICS_RECORDER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/pointer_watcher.h"

namespace gfx {
class Point;
}

namespace ui {
class PointerEvent;
}

namespace ash {

// Form factor of the down event.
// This enum is used to control a UMA histogram buckets. If you change this
// enum, you should update DownEventMetric as well.
enum class DownEventFormFactor {
  kClamshell = 0,
  kTabletModeLandscape,
  kTabletModePortrait,
  kFormFactorCount,
};

// Input type of the down event.
// This enum is used to control a UMA histogram buckets. If you change this
// enum, you should update DownEventMetric as well.
enum class DownEventSource {
  kUnknown = 0,
  kMouse,
  kStylus,
  kTouch,
  kSourceCount,
};

// Input, FormFactor, and Destination Combination of the down event.
// This enum is used to back an UMA histogram and new values should
// be inserted immediately above kCombinationCount.
enum class DownEventMetric {
  kUnknownClamshellOthers = 0,
  kUnknownClamshellBrowser,
  kUnknownClamshellChromeApp,
  kUnknownClamshellArcApp,
  kUnknownTabletLandscapeOthers,
  kUnknownTabletLandscapeBrowser,
  kUnknownTabletLandscapeChromeApp,
  kUnknownTabletLandscapeArcApp,
  kUnknownTabletPortraitOthers,
  kUnknownTabletPortraitBrowser,
  kUnknownTabletPortraitChromeApp,
  kUnknownTabletPortraitArcApp,
  kMouseClamshellOthers,
  kMouseClamshellBrowser,
  kMouseClamshellChromeApp,
  kMouseClamshellArcApp,
  kMouseTabletLandscapeOthers,
  kMouseTabletLandscapeBrowser,
  kMouseTabletLandscapeChromeApp,
  kMouseTabletLandscapeArcApp,
  kMouseTabletPortraitOthers,
  kMouseTabletPortraitBrowser,
  kMouseTabletPortraitChromeApp,
  kMouseTabletPortraitArcApp,
  kStylusClamshellOthers,
  kStylusClamshellBrowser,
  kStylusClamshellChromeApp,
  kStylusClamshellArcApp,
  kStylusTabletLandscapeOthers,
  kStylusTabletLandscapeBrowser,
  kStylusTabletLandscapeChromeApp,
  kStylusTabletLandscapeArcApp,
  kStylusTabletPortraitOthers,
  kStylusTabletPortraitBrowser,
  kStylusTabletPortraitChromeApp,
  kStylusTabletPortraitArcApp,
  kTouchClamshellOthers,
  kTouchClamshellBrowser,
  kTouchClamshellChromeApp,
  kTouchClamshellArcApp,
  kTouchTabletLandscapeOthers,
  kTouchTabletLandscapeBrowser,
  kTouchTabletLandscapeChromeApp,
  kTouchTabletLandscapeArcApp,
  kTouchTabletPortraitOthers,
  kTouchTabletPortraitBrowser,
  kTouchTabletPortraitChromeApp,
  kTouchTabletPortraitArcApp,
  kCombinationCount,
};

// A metrics recorder that records pointer related metrics.
class ASH_EXPORT PointerMetricsRecorder : public views::PointerWatcher {
 public:
  PointerMetricsRecorder();
  ~PointerMetricsRecorder() override;

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PointerMetricsRecorder);
};

}  // namespace ash

#endif  // UI_ASH_METRICS_POINTER_METRICS_RECORDER_H_
