// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_time_tracker.h"

#include "base/metrics/histogram_macros.h"

namespace {
// If a request completes faster than this amount (in ms), then we ignore it.
// Any delays on such a request was negligible.
const int kMinRequestTimeToCareMs = 10;
}  // namespace

ExtensionWebRequestTimeTracker::RequestTimeLog::RequestTimeLog() = default;
ExtensionWebRequestTimeTracker::RequestTimeLog::~RequestTimeLog() = default;

ExtensionWebRequestTimeTracker::ExtensionWebRequestTimeTracker() = default;
ExtensionWebRequestTimeTracker::~ExtensionWebRequestTimeTracker() = default;

void ExtensionWebRequestTimeTracker::LogRequestStartTime(
    int64_t request_id,
    const base::Time& start_time) {
  auto iter = request_time_logs_.find(request_id);
  if (iter != request_time_logs_.end())
    return;

  request_time_logs_[request_id].request_start_time = start_time;
}

void ExtensionWebRequestTimeTracker::LogRequestEndTime(
    int64_t request_id,
    const base::Time& end_time) {
  auto iter = request_time_logs_.find(request_id);
  if (iter == request_time_logs_.end())
    return;

  AnalyzeLogRequest(iter->second, end_time);

  request_time_logs_.erase(iter);
}

void ExtensionWebRequestTimeTracker::AnalyzeLogRequest(
    const RequestTimeLog& log,
    const base::Time& end_time) {
  base::TimeDelta request_duration = end_time - log.request_start_time;

  if (log.block_duration.is_zero())
    return;

  UMA_HISTOGRAM_TIMES("Extensions.NetworkDelay", log.block_duration);

  // Ignore really short requests. Time spent on these is negligible, and any
  // extra delay the extension adds is likely to be noise.
  if (request_duration.InMilliseconds() >= kMinRequestTimeToCareMs) {
    double percentage = log.block_duration.InMillisecondsF() /
                        request_duration.InMillisecondsF();
    UMA_HISTOGRAM_PERCENTAGE("Extensions.NetworkDelayPercentage",
                             static_cast<int>(100 * percentage));
  }
}

void ExtensionWebRequestTimeTracker::IncrementTotalBlockTime(
    int64_t request_id,
    const base::TimeDelta& block_time) {
  auto iter = request_time_logs_.find(request_id);
  if (iter != request_time_logs_.end())
    iter->second.block_duration += block_time;
}

void ExtensionWebRequestTimeTracker::SetRequestCanceled(int64_t request_id) {
  // Canceled requests won't actually hit the network, so we can't compare
  // their request time to the time spent waiting on the extension. Just ignore
  // them.
  request_time_logs_.erase(request_id);
}

void ExtensionWebRequestTimeTracker::SetRequestRedirected(int64_t request_id) {
  // When a request is redirected, we have no way of knowing how long the
  // request would have taken, so we can't say how much an extension slowed
  // down this request. Just ignore it.
  request_time_logs_.erase(request_id);
}
