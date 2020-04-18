// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_STATS_COLLECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_STATS_COLLECTOR_H_

#include <stddef.h>

#include "third_party/blink/renderer/platform/heap/blink_gc.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

// Manages counters and statistics across garbage collection cycles.
//
// Usage:
//   ThreadHeapStatsCollector stats_collector;
//   stats_collector.Start(<BlinkGC::GCReason>);
//   // Use tracer.
//   // Current event is available using stats_collector.current().
//   stats_collector.Stop();
//   // Previous event is available using stats_collector.previous().
class PLATFORM_EXPORT ThreadHeapStatsCollector {
 public:
  // These ids will form human readable names when used in Scopes.
  enum Id {
    kCompleteSweep,
    kEagerSweep,
    kIncrementalMarkingStartMarking,
    kIncrementalMarkingStep,
    kIncrementalMarkingFinalize,
    kIncrementalMarkingFinalizeMarking,
    kInvokePreFinalizers,
    kLazySweepInIdle,
    kLazySweepOnAllocation,
    kAtomicPhaseMarking,
    kVisitDOMWrappers,
    kNumScopeIds,
  };

  static const char* ToString(Id id) {
    switch (id) {
      case Id::kAtomicPhaseMarking:
        return "BlinkGC.AtomicPhaseMarking";
      case Id::kCompleteSweep:
        return "BlinkGC.CompleteSweep";
      case Id::kEagerSweep:
        return "BlinkGC.EagerSweep";
      case Id::kIncrementalMarkingStartMarking:
        return "BlinkGC.IncrementalMarkingStartMarking";
      case Id::kIncrementalMarkingStep:
        return "BlinkGC.IncrementalMarkingStep";
      case Id::kIncrementalMarkingFinalize:
        return "BlinkGC.IncrementalMarkingFinalize";
      case Id::kIncrementalMarkingFinalizeMarking:
        return "BlinkGC.IncrementalMarkingFinalizeMarking";
      case Id::kInvokePreFinalizers:
        return "BlinkGC.InvokePreFinalizers";
      case Id::kLazySweepInIdle:
        return "BlinkGC.LazySweepInIdle";
      case Id::kLazySweepOnAllocation:
        return "BlinkGC.LazySweepOnAllocation";
      case Id::kVisitDOMWrappers:
        return "BlinkGC.VisitDOMWrappers";
      case Id::kNumScopeIds:
        break;
    }
    CHECK(false);
    return nullptr;
  }

  enum TraceDefaultBehavior {
    kEnabled,
    kDisabled,
  };

  // Trace a particular scope. Will emit a trace event and record the time in
  // the corresponding ThreadHeapStatsCollector.
  template <TraceDefaultBehavior default_behavior = kDisabled>
  class PLATFORM_EXPORT InternalScope {
   public:
    template <typename... Args>
    InternalScope(ThreadHeapStatsCollector* tracer, Id id, Args... args)
        : tracer_(tracer),
          start_time_(WTF::CurrentTimeTicksInMilliseconds()),
          id_(id) {
      StartTrace(args...);
    }

    ~InternalScope() {
      TRACE_EVENT_END0(TraceCategory(), ToString(id_));
      tracer_->IncreaseScopeTime(
          id_, WTF::CurrentTimeTicksInMilliseconds() - start_time_);
    }

   private:
    static const char* TraceCategory() {
      return default_behavior == kEnabled
                 ? "blink_gc"
                 : TRACE_DISABLED_BY_DEFAULT("blink_gc");
    }

    void StartTrace() { TRACE_EVENT_BEGIN0(TraceCategory(), ToString(id_)); }

    template <typename Value1>
    void StartTrace(const char* k1, Value1 v1) {
      TRACE_EVENT_BEGIN1(TraceCategory(), ToString(id_), k1, v1);
    }

    template <typename Value1, typename Value2>
    void StartTrace(const char* k1, Value1 v1, const char* k2, Value2 v2) {
      TRACE_EVENT_BEGIN2(TraceCategory(), ToString(id_), k1, v1, k2, v2);
    }

    ThreadHeapStatsCollector* const tracer_;
    const double start_time_;
    const Id id_;
  };

  using Scope = InternalScope<kDisabled>;
  using EnabledScope = InternalScope<kEnabled>;

  struct PLATFORM_EXPORT Event {
    void reset();

    double marking_time_in_ms() const;
    double marking_time_per_byte_in_s() const;
    double sweeping_time_in_ms() const;

    size_t marked_object_size = 0;
    double scope_data[kNumScopeIds] = {0};
    BlinkGC::GCReason reason;
  };

  void Start(BlinkGC::GCReason);
  void Stop();

  void IncreaseScopeTime(Id id, double time) {
    DCHECK(is_started_);
    current_.scope_data[id] += time;
  }

  void IncreaseMarkedObjectSize(size_t);

  bool is_started() const { return is_started_; }
  const Event& current() const { return current_; }
  const Event& previous() const { return previous_; }

 private:
  Event current_;
  Event previous_;
  bool is_started_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_STATS_COLLECTOR_H_
