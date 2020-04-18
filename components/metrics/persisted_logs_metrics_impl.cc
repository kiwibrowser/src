// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/persisted_logs_metrics_impl.h"

#include "base/metrics/histogram_macros.h"

namespace metrics {

void PersistedLogsMetricsImpl::RecordLogReadStatus(
    PersistedLogsMetrics::LogReadStatus status) {
  UMA_HISTOGRAM_ENUMERATION("PrefService.PersistentLogRecallProtobufs", status,
                            PersistedLogsMetrics::END_RECALL_STATUS);
}

void PersistedLogsMetricsImpl::RecordCompressionRatio(
    size_t compressed_size, size_t original_size) {
  UMA_HISTOGRAM_PERCENTAGE(
      "UMA.ProtoCompressionRatio",
      static_cast<int>(100 * compressed_size / original_size));
}

void PersistedLogsMetricsImpl::RecordDroppedLogSize(size_t size) {
  UMA_HISTOGRAM_COUNTS("UMA.Large Accumulated Log Not Persisted",
                       static_cast<int>(size));
}

void PersistedLogsMetricsImpl::RecordDroppedLogsNum(int dropped_logs_num) {
  UMA_HISTOGRAM_COUNTS("UMA.UnsentLogs.Dropped", dropped_logs_num);
}

}  // namespace metrics
