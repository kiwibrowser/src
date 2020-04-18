// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/stability_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace browser_watcher {

void LogCollectOnCrashEvent(CollectOnCrashEvent event) {
  UMA_HISTOGRAM_ENUMERATION("ActivityTracker.CollectCrash.Event", event,
                            CollectOnCrashEvent::kCollectOnCrashEventMax);
}

void LogStabilityRecordEvent(StabilityRecordEvent event) {
  UMA_HISTOGRAM_ENUMERATION("ActivityTracker.Record.Event", event,
                            StabilityRecordEvent::kStabilityRecordEventMax);
}

}  // namespace browser_watcher
