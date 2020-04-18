// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_H_

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ExecutionContext;
class ExceptionState;
class Performance;
class PerformanceObserver;
class PerformanceObserverInit;
class V8PerformanceObserverCallback;

using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;

class CORE_EXPORT PerformanceObserver final
    : public ScriptWrappable,
      public ActiveScriptWrappable<PerformanceObserver>,
      public ContextClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(PerformanceObserver);
  friend class Performance;
  friend class PerformanceTest;
  friend class PerformanceObserverTest;

 public:
  static PerformanceObserver* Create(ScriptState*,
                                     V8PerformanceObserverCallback*);
  static void ResumeSuspendedObservers();

  void observe(const PerformanceObserverInit&, ExceptionState&);
  void disconnect();
  PerformanceEntryVector takeRecords();
  void EnqueuePerformanceEntry(PerformanceEntry&);
  PerformanceEntryTypeMask FilterOptions() const { return filter_options_; }

  // ScriptWrappable
  bool HasPendingActivity() const final;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  PerformanceObserver(ExecutionContext*,
                      Performance*,
                      V8PerformanceObserverCallback*);
  void Deliver();
  bool ShouldBeSuspended() const;

  Member<ExecutionContext> execution_context_;
  TraceWrapperMember<V8PerformanceObserverCallback> callback_;
  WeakMember<Performance> performance_;
  PerformanceEntryVector performance_entries_;
  PerformanceEntryTypeMask filter_options_;
  bool is_registered_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_H_
