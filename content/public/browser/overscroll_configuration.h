// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_
#define CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT OverscrollConfig {
 public:
  // Determines overscroll history navigation mode according to
  // --overscroll-history-navigation flag.
  enum class HistoryNavigationMode {
    // History navigation is disabled.
    kDisabled,

    // History navigation is enabled and uses the UI with parallax effect and
    // screenshots.
    kParallaxUi,

    // History navigation is enabled and uses the simplified UI.
    kSimpleUi,
  };

  // Determines pull-to-refresh mode according to --pull-to-refresh flag.
  enum class PullToRefreshMode {
    // Pull-to-refresh is disabled.
    kDisabled,

    // Pull-to-refresh is enabled for both touchscreen and touchpad.
    kEnabled,

    // Pull-to-refresh is enabled only for touchscreen.
    kEnabledTouchschreen,
  };

  // Specifies an overscroll controller threshold.
  enum class Threshold {
    // Threshold to complete touchpad overscroll, in terms of the percentage of
    // the display size.
    kCompleteTouchpad,

    // Threshold to complete touchscreen overscroll, in terms of the percentage
    // of the display size.
    kCompleteTouchscreen,

    // Threshold to start touchpad overscroll, in DIPs.
    kStartTouchpad,

    // Threshold to start touchscreen overscroll, in DIPs.
    kStartTouchscreen,
  };

  static HistoryNavigationMode GetHistoryNavigationMode();
  static PullToRefreshMode GetPullToRefreshMode();

  static float GetThreshold(Threshold threshold);

  static bool TouchpadOverscrollHistoryNavigationEnabled();

  static base::TimeDelta MaxInertialEventsBeforeOverscrollCancellation();

 private:
  friend class ScopedHistoryNavigationMode;
  friend class ScopedPullToRefreshMode;
  friend class OverscrollControllerTest;

  // Helper functions used by |ScopedHistoryNavigationMode| to set and reset
  // mode in tests.
  static void SetHistoryNavigationMode(HistoryNavigationMode mode);
  static void ResetHistoryNavigationMode();

  // Helper functions used by |ScopedPullToRefreshMode| to set and reset mode in
  // tests.
  static void SetPullToRefreshMode(PullToRefreshMode mode);
  static void ResetPullToRefreshMode();

  // Helper functions to reset TouchpadOverscrollHistoryNavigationEnabled in
  // tests.
  static void ResetTouchpadOverscrollHistoryNavigationEnabled();

  DISALLOW_IMPLICIT_CONSTRUCTORS(OverscrollConfig);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_
