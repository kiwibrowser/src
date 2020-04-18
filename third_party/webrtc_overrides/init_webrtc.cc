// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "init_webrtc.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "third_party/webrtc/rtc_base/event_tracer.h"
#include "third_party/webrtc/system_wrappers/include/cpu_info.h"
#include "third_party/webrtc_overrides/rtc_base/logging.h"

const unsigned char* GetCategoryGroupEnabled(const char* category_group) {
  return TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category_group);
}

void AddTraceEvent(char phase,
                   const unsigned char* category_group_enabled,
                   const char* name,
                   unsigned long long id,
                   int num_args,
                   const char** arg_names,
                   const unsigned char* arg_types,
                   const unsigned long long* arg_values,
                   unsigned char flags) {
  TRACE_EVENT_API_ADD_TRACE_EVENT(
      phase, category_group_enabled, name, trace_event_internal::kGlobalScope,
      id, num_args, arg_names, arg_types, arg_values, NULL, flags);
}

namespace webrtc {

// Define webrtc::metrics functions to provide webrtc with implementations.
namespace metrics {

// This class doesn't actually exist, so don't go looking for it :)
// This type is just fwd declared here in order to use it as an opaque type
// between the Histogram functions in this file.
class Histogram;

Histogram* HistogramFactoryGetCounts(
    const std::string& name, int min, int max, int bucket_count) {
  return reinterpret_cast<Histogram*>(
      base::Histogram::FactoryGet(name, min, max, bucket_count,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

Histogram* HistogramFactoryGetCountsLinear(
    const std::string& name, int min, int max, int bucket_count) {
  return reinterpret_cast<Histogram*>(
      base::LinearHistogram::FactoryGet(name, min, max, bucket_count,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

Histogram* HistogramFactoryGetEnumeration(
    const std::string& name, int boundary) {
  return reinterpret_cast<Histogram*>(
      base::LinearHistogram::FactoryGet(name, 1, boundary, boundary + 1,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

const char* GetHistogramName(Histogram* histogram_pointer) {
  base::HistogramBase* ptr =
      reinterpret_cast<base::HistogramBase*>(histogram_pointer);
  return ptr->histogram_name();
}

void HistogramAdd(Histogram* histogram_pointer, int sample) {
  base::HistogramBase* ptr =
      reinterpret_cast<base::HistogramBase*>(histogram_pointer);
  ptr->Add(sample);
}
}  // namespace metrics
}  // namespace webrtc

bool InitializeWebRtcModule() {
  // Workaround for crbug.com/176522
  // On Linux, we can't fetch the number of cores after the sandbox has been
  // initialized, so we call DetectNumberOfCores() here, to cache the value.
  webrtc::CpuInfo::DetectNumberOfCores();
  webrtc::SetupEventTracer(&GetCategoryGroupEnabled, &AddTraceEvent);
  return true;
}
