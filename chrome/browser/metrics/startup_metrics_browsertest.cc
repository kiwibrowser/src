// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>

#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"

using StartupMetricsTest = InProcessBrowserTest;

namespace {

constexpr const char* kStartupMetrics[] = {
    "Startup.BrowserMainToRendererMain",
    "Startup.BrowserOpenTabs",
    "Startup.BrowserWindow.FirstPaint",
    "Startup.BrowserWindow.FirstPaint.CompositingEnded",
    "Startup.BrowserWindowDisplay",
    "Startup.FirstWebContents.MainFrameLoad2",
    "Startup.FirstWebContents.MainNavigationFinished",
    "Startup.FirstWebContents.MainNavigationStart",
    "Startup.FirstWebContents.NonEmptyPaint2",
    "Startup.FirstWebContents.RenderProcessHostInit.ToNonEmptyPaint",

    // The following histograms depend on normal browser startup through
    // BrowserMain and are as such not caught by this browser test.
    // "Startup.BrowserMessageLoopStartHardFaultCount",
    // "Startup.BrowserMessageLoopStartTime",
    // "Startup.BrowserMessageLoopStartTimeFromMainEntry2",
    // "Startup.BrowserMessageLoopStartTimeFromMainEntry3",
    // "Startup.LoadTime.ExeMainToDllMain2",
    // "Startup.LoadTime.ProcessCreateToDllMain2",
    // "Startup.LoadTime.ProcessCreateToExeMain2",
    // "Startup.SystemUptime",
    // "Startup.Temperature",
};

}  // namespace

// Verify that startup histograms are logged on browser startup.
IN_PROC_BROWSER_TEST_F(StartupMetricsTest, ReportsValues) {
  for (auto* const histogram : kStartupMetrics) {
    while (!base::StatisticsRecorder::FindHistogram(histogram))
      base::RunLoop().RunUntilIdle();
  }
}
