// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_TASK_ATTRIBUTION_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_TASK_ATTRIBUTION_TIMING_H_

#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class TaskAttributionTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Used when the LongTaskV2 flag is enabled.
  static TaskAttributionTiming* Create(String type,
                                       String container_type,
                                       String container_src,
                                       String container_id,
                                       String container_name,
                                       double start_time,
                                       double finish_time,
                                       String script_url) {
    return new TaskAttributionTiming(type, container_type, container_src,
                                     container_id, container_name, start_time,
                                     finish_time, script_url);
  }

  // Used when the LongTaskV2 flag is disabled.
  static TaskAttributionTiming* Create(String type,
                                       String container_type,
                                       String container_src,
                                       String container_id,
                                       String container_name) {
    return new TaskAttributionTiming(type, container_type, container_src,
                                     container_id, container_name, 0.0, 0.0,
                                     g_empty_string);
  }
  String containerType() const;
  String containerSrc() const;
  String containerId() const;
  String containerName() const;
  String scriptURL() const;

  void Trace(blink::Visitor*) override;

  ~TaskAttributionTiming() override;

 private:
  TaskAttributionTiming(String type,
                        String container_type,
                        String container_src,
                        String container_id,
                        String container_name,
                        double start_time,
                        double finish_time,
                        String script_url);
  void BuildJSONValue(V8ObjectBuilder&) const override;

  String container_type_;
  String container_src_;
  String container_id_;
  String container_name_;
  String script_url_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_TASK_ATTRIBUTION_TIMING_H_
