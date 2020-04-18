// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_PERSISTED_LOGS_METRICS_IMPL_H_
#define COMPONENTS_METRICS_PERSISTED_LOGS_METRICS_IMPL_H_

#include "base/macros.h"
#include "components/metrics/persisted_logs_metrics.h"

namespace metrics {

// Implementation for recording metrics from PersistedLogs.
class PersistedLogsMetricsImpl : public PersistedLogsMetrics {
 public:
  PersistedLogsMetricsImpl() {}
  ~PersistedLogsMetricsImpl() override {}

  // PersistedLogsMetrics:
  void RecordLogReadStatus(PersistedLogsMetrics::LogReadStatus status) override;
  void RecordCompressionRatio(
    size_t compressed_size, size_t original_size) override;
  void RecordDroppedLogSize(size_t size) override;
  void RecordDroppedLogsNum(int dropped_logs_num) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistedLogsMetricsImpl);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_PERSISTED_LOGS_METRICS_IMPL_H_
