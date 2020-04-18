// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_SUB_TASK_ATTRIBUTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_SUB_TASK_ATTRIBUTION_H_

#include <memory>

#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class SubTaskAttribution {
 public:
  using EntriesVector = Vector<std::unique_ptr<SubTaskAttribution>>;

  static std::unique_ptr<SubTaskAttribution> Create(String sub_task_name,
                                                    String script_url,
                                                    TimeTicks start_time,
                                                    TimeDelta duration) {
    return std::make_unique<SubTaskAttribution>(sub_task_name, script_url,
                                                start_time, duration);
  }
  SubTaskAttribution(String sub_task_name,
                     String script_url,
                     TimeTicks start_time,
                     TimeDelta duration);
  inline String subTaskName() const { return sub_task_name_; }
  inline String scriptURL() const { return script_url_; }
  inline TimeTicks startTime() const { return start_time_; }
  inline TimeDelta duration() const { return duration_; }

  inline DOMHighResTimeStamp highResStartTime() const {
    return high_res_start_time_;
  }
  inline DOMHighResTimeStamp highResDuration() const {
    return high_res_duration_;
  }
  void setHighResStartTime(DOMHighResTimeStamp high_res_start_time) {
    high_res_start_time_ = high_res_start_time;
  }
  void setHighResDuration(DOMHighResTimeStamp high_res_duration) {
    high_res_duration_ = high_res_duration;
  }

 private:
  String sub_task_name_;
  String script_url_;
  TimeTicks start_time_;
  TimeDelta duration_;
  DOMHighResTimeStamp high_res_start_time_;
  DOMHighResTimeStamp high_res_duration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_SUB_TASK_ATTRIBUTION_H_
