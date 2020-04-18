// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_EVENT_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_EVENT_TIMING_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"

namespace blink {

class CORE_EXPORT PerformanceEventTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PerformanceEventTiming* Create(const String& event_type,
                                        DOMHighResTimeStamp start_time,
                                        DOMHighResTimeStamp processing_start,
                                        DOMHighResTimeStamp processing_end,
                                        bool cancelable);

  static PerformanceEventTiming* CreateFirstInputTiming(
      PerformanceEventTiming* entry);

  ~PerformanceEventTiming() override;

  bool cancelable() const { return cancelable_; }

  DOMHighResTimeStamp processingStart() const;
  DOMHighResTimeStamp processingEnd() const;

  void SetDuration(double duration);

  void BuildJSONValue(V8ObjectBuilder&) const override;

  void Trace(blink::Visitor*) override;

 private:
  PerformanceEventTiming(const String& event_type,
                         const String& entry_type,
                         DOMHighResTimeStamp start_time,
                         DOMHighResTimeStamp processing_start,
                         DOMHighResTimeStamp processing_end,
                         bool cancelable);
  DOMHighResTimeStamp processing_start_;
  DOMHighResTimeStamp processing_end_;
  bool cancelable_;
};
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_EVENT_TIMING_H_
