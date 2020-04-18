// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/keepalive_statistics_recorder.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"

namespace network {

KeepaliveStatisticsRecorder::KeepaliveStatisticsRecorder() {
  UMA_HISTOGRAM_COUNTS_1000(
      "Net.KeepaliveStatisticsRecorder.PeakInflightRequests2", 0);
}
KeepaliveStatisticsRecorder::~KeepaliveStatisticsRecorder() = default;

void KeepaliveStatisticsRecorder::Register(int process_id) {
  auto it = per_process_records_.find(process_id);
  if (it == per_process_records_.end()) {
    per_process_records_.insert(std::make_pair(process_id, PerProcessStats()));
    UMA_HISTOGRAM_COUNTS_100(
        "Net.KeepaliveStatisticsRecorder.PeakInflightRequestsPerProcess2", 0);
    return;
  }

  ++it->second.num_registrations;
}

void KeepaliveStatisticsRecorder::Unregister(int process_id) {
  auto it = per_process_records_.find(process_id);
  DCHECK(it != per_process_records_.end());

  if (it->second.num_registrations == 1) {
    UMA_HISTOGRAM_COUNTS_100(
        "Net.KeepaliveStatisticsRecorder.PeakInflightRequestsPerProcess",
        it->second.peak_inflight_requests);

    per_process_records_.erase(it);
    return;
  }
  --it->second.num_registrations;
}

void KeepaliveStatisticsRecorder::OnLoadStarted(int process_id) {
  auto it = per_process_records_.find(process_id);
  if (it != per_process_records_.end()) {
    ++it->second.num_inflight_requests;
    if (it->second.peak_inflight_requests < it->second.num_inflight_requests) {
      it->second.peak_inflight_requests = it->second.num_inflight_requests;
      UMA_HISTOGRAM_COUNTS_100(
          "Net.KeepaliveStatisticsRecorder.PeakInflightRequestsPerProcess2",
          it->second.peak_inflight_requests);
    }
  }
  ++num_inflight_requests_;
  if (peak_inflight_requests_ < num_inflight_requests_) {
    peak_inflight_requests_ = num_inflight_requests_;
    UMA_HISTOGRAM_COUNTS_1000(
        "Net.KeepaliveStatisticsRecorder.PeakInflightRequests2",
        peak_inflight_requests_);
  }
}

void KeepaliveStatisticsRecorder::OnLoadFinished(int process_id) {
  auto it = per_process_records_.find(process_id);
  if (it != per_process_records_.end())
    --it->second.num_inflight_requests;
  --num_inflight_requests_;
}

int KeepaliveStatisticsRecorder::NumInflightRequestsPerProcess(
    int process_id) const {
  auto it = per_process_records_.find(process_id);
  if (it == per_process_records_.end())
    return 0;
  return it->second.num_inflight_requests;
}

bool KeepaliveStatisticsRecorder::HasRecordForProcess(int process_id) const {
  auto it = per_process_records_.find(process_id);
  if (it != per_process_records_.end()) {
    DCHECK(it->second.num_registrations > 0);
    return true;
  }
  return false;
}

}  // namespace network
