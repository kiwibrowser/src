// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_STABILITY_METRICS_H_
#define COMPONENTS_BROWSER_WATCHER_STABILITY_METRICS_H_

namespace browser_watcher {

// DO NOT REMOVE OR REORDER VALUES. This is logged persistently in a histogram.
enum class CollectOnCrashEvent {
  kCollectAttempt,
  kUserDataDirNotEmpty,
  kPathExists,
  kReportExtractionSuccess,
  kPmaSetDeletedFailed,
  kOpenForDeleteFailed,
  kSuccess,
  // New values go here.
  kCollectOnCrashEventMax
};

// DO NOT REMOVE OR REORDER VALUES. This is logged persistently in a histogram.
enum class StabilityRecordEvent {
  kRecordAttempt,
  kStabilityDirectoryExists,
  kGotStabilityPath,
  kGotTracker,
  kMarkDeleted,
  kMarkDeletedGotFile,
  kOpenForDeleteFailed,
  // New values go here.
  kStabilityRecordEventMax
};

void LogCollectOnCrashEvent(CollectOnCrashEvent event);
void LogStabilityRecordEvent(StabilityRecordEvent event);

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_STABILITY_METRICS_H_
