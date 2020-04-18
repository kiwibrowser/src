// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_CHROME_BROWSER_MAIN_EXTRA_PARTS_METRICS_H_
#define CHROME_BROWSER_METRICS_CHROME_BROWSER_MAIN_EXTRA_PARTS_METRICS_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "ui/display/display_observer.h"

class ChromeBrowserMainParts;

namespace chrome {
void AddMetricsExtraParts(ChromeBrowserMainParts* main_parts);
}

namespace ui {
class InputDeviceEventObserver;
}  // namespace ui

class ChromeBrowserMainExtraPartsMetrics : public ChromeBrowserMainExtraParts,
                                           public display::DisplayObserver {
 public:
  ChromeBrowserMainExtraPartsMetrics();
  ~ChromeBrowserMainExtraPartsMetrics() override;

  // Overridden from ChromeBrowserMainExtraParts:
  void PreProfileInit() override;
  void PreBrowserStart() override;
  void PostBrowserStart() override;

 private:
#if defined(OS_MACOSX)
  // Records Mac specific metrics.
  void RecordMacMetrics();
#endif  // defined(OS_MACOSX)

  // DisplayObserver overrides.
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // If the number of displays has changed, emit a UMA metric.
  void EmitDisplaysChangedMetric();

  // A cached value for the number of displays.
  int display_count_;

  // True iff |this| instance is registered as an observer of the native
  // screen.
  bool is_screen_observer_;

#if defined(USE_OZONE) || defined(USE_X11)
  std::unique_ptr<ui::InputDeviceEventObserver> input_device_event_observer_;
#endif  // defined(USE_OZONE) || defined(USE_X11)

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsMetrics);
};

#endif  // CHROME_BROWSER_METRICS_CHROME_BROWSER_MAIN_EXTRA_PARTS_METRICS_H_
