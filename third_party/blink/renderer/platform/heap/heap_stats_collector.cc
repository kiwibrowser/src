// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/heap_stats_collector.h"

#include "base/logging.h"

namespace blink {

void ThreadHeapStatsCollector::IncreaseMarkedObjectSize(size_t size) {
  DCHECK(is_started_);
  current_.marked_object_size += size;
}

void ThreadHeapStatsCollector::Start(BlinkGC::GCReason reason) {
  DCHECK(!is_started_);
  is_started_ = true;
  current_.reason = reason;
}

void ThreadHeapStatsCollector::Stop() {
  is_started_ = false;
  previous_ = std::move(current_);
  current_.reset();
}

void ThreadHeapStatsCollector::Event::reset() {
  marked_object_size = 0;
  memset(scope_data, 0, sizeof(scope_data));
  reason = BlinkGC::kTesting;
}

double ThreadHeapStatsCollector::Event::marking_time_in_ms() const {
  return scope_data[kIncrementalMarkingStartMarking] +
         scope_data[kIncrementalMarkingStep] +
         scope_data[kIncrementalMarkingFinalizeMarking] +
         scope_data[kAtomicPhaseMarking];
}

double ThreadHeapStatsCollector::Event::marking_time_per_byte_in_s() const {
  return marked_object_size ? marking_time_in_ms() / 1000 / marked_object_size
                            : 0.0;
}

double ThreadHeapStatsCollector::Event::sweeping_time_in_ms() const {
  return scope_data[kCompleteSweep] + scope_data[kEagerSweep] +
         scope_data[kLazySweepInIdle] + scope_data[kLazySweepOnAllocation];
}

}  // namespace blink
