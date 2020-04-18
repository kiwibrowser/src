/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_resource_timing_info.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_double.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/loader/frame_loader_types.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/core/timing/performance_navigation_timing.h"
#include "third_party/blink/renderer/core/timing/performance_paint_timing.h"
#include "third_party/blink/renderer/core/timing/sub_task_attribution.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/linked_hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class DoubleOrPerformanceMarkOptions;
class ExceptionState;
class MemoryInfo;
class PerformanceNavigation;
class PerformanceObserver;
class PerformanceMark;
class PerformanceMeasure;
class PerformanceTiming;
class ResourceResponse;
class ResourceTimingInfo;
class SecurityOrigin;
class UserTiming;
class ScriptState;
class ScriptValue;
class SubTaskAttribution;
class V8ObjectBuilder;
class PerformanceEventTiming;
class StringOrDoubleOrPerformanceMeasureOptions;

using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;

class CORE_EXPORT Performance : public EventTargetWithInlineData {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~Performance() override;

  const AtomicString& InterfaceName() const override;

  // Overriden by WindowPerformance but not by WorkerPerformance.
  virtual PerformanceTiming* timing() const;
  virtual PerformanceNavigation* navigation() const;
  virtual MemoryInfo* memory() const;

  virtual void UpdateLongTaskInstrumentation() {}

  // Reduce the resolution to prevent timing attacks. See:
  // http://www.w3.org/TR/hr-time-2/#privacy-security
  static double ClampTimeResolution(double time_seconds);

  static DOMHighResTimeStamp MonotonicTimeToDOMHighResTimeStamp(
      TimeTicks time_origin,
      TimeTicks monotonic_time,
      bool allow_negative_value);

  // Translate given platform monotonic time in seconds into a high resolution
  // DOMHighResTimeStamp in milliseconds. The result timestamp is relative to
  // document's time origin and has a time resolution that is safe for
  // exposing to web.
  DOMHighResTimeStamp MonotonicTimeToDOMHighResTimeStamp(TimeTicks) const;
  DOMHighResTimeStamp now() const;

  // High Resolution Time Level 3 timeOrigin.
  // (https://www.w3.org/TR/hr-time-3/#dom-performance-timeorigin)
  DOMHighResTimeStamp timeOrigin() const;

  // Internal getter method for the time origin value.
  double GetTimeOrigin() const { return TimeTicksInSeconds(time_origin_); }

  PerformanceEntryVector getEntries();
  PerformanceEntryVector getEntriesByType(const String& entry_type);
  PerformanceEntryVector getEntriesByName(const String& name,
                                          const String& entry_type);

  void clearResourceTimings();
  void setResourceTimingBufferSize(unsigned);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(resourcetimingbufferfull);

  void AddLongTaskTiming(
      TimeTicks start_time,
      TimeTicks end_time,
      const String& name,
      const String& culprit_frame_src,
      const String& culprit_frame_id,
      const String& culprit_frame_name,
      const SubTaskAttribution::EntriesVector& sub_task_attributions);

  // Generates and add a performance entry for the given ResourceTimingInfo.
  // |overridden_initiator_type| allows the initiator type to be overridden to
  // the frame element name for the main resource.
  void GenerateAndAddResourceTiming(
      const ResourceTimingInfo&,
      const AtomicString& overridden_initiator_type = g_null_atom);
  // Generates timing info suitable for appending to the performance entries of
  // a context with |origin|. This should be rarely used; most callsites should
  // prefer the convenience method |GenerateAndAddResourceTiming()|.
  static WebResourceTimingInfo GenerateResourceTiming(
      const SecurityOrigin& destination_origin,
      const ResourceTimingInfo&,
      ExecutionContext& context_for_use_counter);
  void AddResourceTiming(const WebResourceTimingInfo&,
                         const AtomicString& initiator_type = g_null_atom);

  void NotifyNavigationTimingToObservers();

  void AddFirstPaintTiming(TimeTicks start_time);

  void AddFirstContentfulPaintTiming(TimeTicks start_time);

  bool IsEventTimingBufferFull() const;
  void AddEventTimingBuffer(PerformanceEventTiming&);
  unsigned EventTimingBufferSize() const;
  void clearEventTimings();
  void setEventTimingBufferMaxSize(unsigned);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(eventtimingbufferfull);

  PerformanceMark* mark(ScriptState*, const String& mark_name, ExceptionState&);

  PerformanceMark* mark(
      ScriptState*,
      const String& mark_name,
      DoubleOrPerformanceMarkOptions& start_time_or_mark_options,
      ExceptionState&);

  void clearMarks(const String& mark_name);

  PerformanceMeasure* measure(ScriptState*,
                              const String& measure_name,
                              ExceptionState&);

  PerformanceMeasure* measure(
      ScriptState*,
      const String& measure_name,
      const StringOrDoubleOrPerformanceMeasureOptions& start_or_options,
      ExceptionState&);

  PerformanceMeasure* measure(
      ScriptState*,
      const String& measure_name,
      const StringOrDoubleOrPerformanceMeasureOptions& start_or_options,
      const StringOrDouble& end,
      ExceptionState&);

  void clearMeasures(const String& measure_name);

  void UnregisterPerformanceObserver(PerformanceObserver&);
  void RegisterPerformanceObserver(PerformanceObserver&);
  void UpdatePerformanceObserverFilterOptions();
  void ActivateObserver(PerformanceObserver&);
  void ResumeSuspendedObservers();

  static bool AllowsTimingRedirect(const Vector<ResourceResponse>&,
                                   const ResourceResponse&,
                                   const SecurityOrigin&,
                                   ExecutionContext*);

  ScriptValue toJSONForBinding(ScriptState*) const;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  static bool PassesTimingAllowCheck(const ResourceResponse&,
                                     const SecurityOrigin&,
                                     const AtomicString&,
                                     ExecutionContext*);

  void AddPaintTiming(PerformancePaintTiming::PaintType, TimeTicks start_time);

  PerformanceMeasure* measureInternal(
      ScriptState*,
      const String& measure_name,
      const StringOrDoubleOrPerformanceMeasureOptions& start,
      const StringOrDouble& end,
      bool end_is_empty,
      ExceptionState&);

  PerformanceMeasure* measureInternal(ScriptState*,
                                      const String& measure_name,
                                      const StringOrDouble& start,
                                      const StringOrDouble& end,
                                      const ScriptValue& detail,
                                      ExceptionState&);

 protected:
  Performance(TimeTicks time_origin,
              scoped_refptr<base::SingleThreadTaskRunner>);

  // Expect WindowPerformance to override this method,
  // WorkerPerformance doesn't have to override this.
  virtual PerformanceNavigationTiming* CreateNavigationTimingInstance() {
    return nullptr;
  }

  bool IsResourceTimingBufferFull();
  void AddResourceTimingBuffer(PerformanceEntry&);

  void NotifyObserversOfEntry(PerformanceEntry&) const;
  void NotifyObserversOfEntries(PerformanceEntryVector&);
  bool HasObserverFor(PerformanceEntry::EntryType) const;

  void DeliverObservationsTimerFired(TimerBase*);

  virtual void BuildJSONValue(V8ObjectBuilder&) const;

  PerformanceEntryVector frame_timing_buffer_;
  unsigned frame_timing_buffer_size_;
  PerformanceEntryVector resource_timing_buffer_;
  unsigned resource_timing_buffer_size_;
  PerformanceEntryVector event_timing_buffer_;
  unsigned event_timing_buffer_max_size_;
  Member<PerformanceEntry> navigation_timing_;
  Member<UserTiming> user_timing_;
  Member<PerformanceEntry> first_paint_timing_;
  Member<PerformanceEntry> first_contentful_paint_timing_;
  Member<PerformanceEventTiming> first_input_timing_;

  TimeTicks time_origin_;

  PerformanceEntryTypeMask observer_filter_options_;
  HeapLinkedHashSet<TraceWrapperMember<PerformanceObserver>> observers_;
  HeapLinkedHashSet<Member<PerformanceObserver>> active_observers_;
  HeapLinkedHashSet<Member<PerformanceObserver>> suspended_observers_;
  TaskRunnerTimer<Performance> deliver_observations_timer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_H_
