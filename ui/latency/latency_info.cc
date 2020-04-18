// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/latency_info.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"

namespace {

const size_t kMaxLatencyInfoNumber = 100;

const char* GetComponentName(ui::LatencyComponentType type) {
#define CASE_TYPE(t) case ui::t:  return #t
  switch (type) {
    CASE_TYPE(INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT);
    CASE_TYPE(LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_UI_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT);
    CASE_TYPE(TAB_SHOW_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT);
    CASE_TYPE(DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT);
    CASE_TYPE(INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL);
    CASE_TYPE(INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT);
    CASE_TYPE(INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT);
    default:
      DLOG(WARNING) << "Unhandled LatencyComponentType.\n";
      break;
  }
#undef CASE_TYPE
  return "unknown";
}

bool IsInputLatencyBeginComponent(ui::LatencyComponentType type) {
  return type == ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT;
}

bool IsTraceBeginComponent(ui::LatencyComponentType type) {
  return (IsInputLatencyBeginComponent(type) ||
          type == ui::LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT);
}

bool IsTraceEndComponent(ui::LatencyComponentType type) {
  switch (type) {
    case ui::INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT:
    case ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT:
    case ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT:
    case ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT:
    case ui::INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT:
      return true;
    default:
      return false;
  }
}

// This class is for converting latency info to trace buffer friendly format.
class LatencyInfoTracedValue
    : public base::trace_event::ConvertableToTraceFormat {
 public:
  static std::unique_ptr<ConvertableToTraceFormat> FromValue(
      std::unique_ptr<base::Value> value);

  void AppendAsTraceFormat(std::string* out) const override;

 private:
  explicit LatencyInfoTracedValue(base::Value* value);
  ~LatencyInfoTracedValue() override;

  std::unique_ptr<base::Value> value_;

  DISALLOW_COPY_AND_ASSIGN(LatencyInfoTracedValue);
};

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
LatencyInfoTracedValue::FromValue(std::unique_ptr<base::Value> value) {
  return std::unique_ptr<base::trace_event::ConvertableToTraceFormat>(
      new LatencyInfoTracedValue(value.release()));
}

LatencyInfoTracedValue::~LatencyInfoTracedValue() {
}

void LatencyInfoTracedValue::AppendAsTraceFormat(std::string* out) const {
  std::string tmp;
  base::JSONWriter::Write(*value_, &tmp);
  *out += tmp;
}

LatencyInfoTracedValue::LatencyInfoTracedValue(base::Value* value)
    : value_(value) {
}

const char kTraceCategoriesForAsyncEvents[] = "benchmark,latencyInfo,rail";

struct LatencyInfoEnabledInitializer {
  LatencyInfoEnabledInitializer() :
      latency_info_enabled(TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(
          kTraceCategoriesForAsyncEvents)) {
  }

  const unsigned char* latency_info_enabled;
};

static base::LazyInstance<LatencyInfoEnabledInitializer>::Leaky
  g_latency_info_enabled = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace ui {

LatencyInfo::LatencyInfo() : LatencyInfo(SourceEventType::UNKNOWN) {}

LatencyInfo::LatencyInfo(SourceEventType type)
    : trace_id_(-1),
      ukm_source_id_(ukm::kInvalidSourceId),
      coalesced_(false),
      began_(false),
      terminated_(false),
      source_event_type_(type) {}

LatencyInfo::LatencyInfo(const LatencyInfo& other) = default;

LatencyInfo::~LatencyInfo() {}

LatencyInfo::LatencyInfo(int64_t trace_id, bool terminated)
    : trace_id_(trace_id),
      ukm_source_id_(ukm::kInvalidSourceId),
      coalesced_(false),
      began_(false),
      terminated_(terminated),
      source_event_type_(SourceEventType::UNKNOWN) {}

bool LatencyInfo::Verify(const std::vector<LatencyInfo>& latency_info,
                         const char* referring_msg) {
  if (latency_info.size() > kMaxLatencyInfoNumber) {
    LOG(ERROR) << referring_msg << ", LatencyInfo vector size "
               << latency_info.size() << " is too big.";
    TRACE_EVENT_INSTANT1("input,benchmark", "LatencyInfo::Verify Fails",
                         TRACE_EVENT_SCOPE_GLOBAL,
                         "size", latency_info.size());
    return false;
  }
  return true;
}

void LatencyInfo::TraceIntermediateFlowEvents(
    const std::vector<LatencyInfo>& latency_info,
    const char* event_name) {
  for (auto& latency : latency_info) {
    if (latency.trace_id() == -1)
      continue;
    TRACE_EVENT_WITH_FLOW1("input,benchmark", "LatencyInfo.Flow",
                           TRACE_ID_DONT_MANGLE(latency.trace_id()),
                           TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                           "step", event_name);
  }
}

void LatencyInfo::CopyLatencyFrom(const LatencyInfo& other,
                                  LatencyComponentType type) {
  // Don't clobber an existing trace_id_ or ukm_source_id_.
  if (trace_id_ == -1) {
    DCHECK_EQ(ukm_source_id_, ukm::kInvalidSourceId);
    DCHECK(latency_components().empty());
    trace_id_ = other.trace_id();
    ukm_source_id_ = other.ukm_source_id();
  } else {
    DCHECK_NE(ukm_source_id_, ukm::kInvalidSourceId);
  }

  for (const auto& lc : other.latency_components()) {
    if (lc.first.first == type) {
      AddLatencyNumberWithTimestamp(lc.first.first,
                                    lc.first.second,
                                    lc.second.event_time,
                                    lc.second.event_count);
    }
  }

  coalesced_ = other.coalesced();
  // TODO(tdresser): Ideally we'd copy |began_| here as well, but |began_|
  // isn't very intuitive, and we can actually begin multiple times across
  // copied events.
  terminated_ = other.terminated();

  snapshots_ = other.Snapshots();
}

void LatencyInfo::AddNewLatencyFrom(const LatencyInfo& other) {
  // Don't clobber an existing trace_id_ or ukm_source_id_.
  if (trace_id_ == -1) {
    trace_id_ = other.trace_id();
  }

  if (ukm_source_id_ == ukm::kInvalidSourceId) {
    ukm_source_id_ = other.ukm_source_id();
  }

  for (const auto& lc : other.latency_components()) {
    if (!FindLatency(lc.first.first, lc.first.second, NULL)) {
      AddLatencyNumberWithTimestamp(lc.first.first,
                                    lc.first.second,
                                    lc.second.event_time,
                                    lc.second.event_count);
    }
  }

  coalesced_ = other.coalesced();
  // TODO(tdresser): Ideally we'd copy |began_| here as well, but |began_| isn't
  // very intuitive, and we can actually begin multiple times across copied
  // events.
  terminated_ = other.terminated();

  snapshots_ = other.Snapshots();
}

void LatencyInfo::AddLatencyNumber(LatencyComponentType component, int64_t id) {
  AddLatencyNumberWithTimestampImpl(component, id, base::TimeTicks::Now(), 1,
                                    nullptr);
}

void LatencyInfo::AddLatencyNumberWithTraceName(
    LatencyComponentType component,
    int64_t id,
    const char* trace_name_str) {
  AddLatencyNumberWithTimestampImpl(component, id, base::TimeTicks::Now(), 1,
                                    trace_name_str);
}

void LatencyInfo::AddLatencyNumberWithTimestamp(
    LatencyComponentType component,
    int64_t id,
    base::TimeTicks time,
    uint32_t event_count) {
  AddLatencyNumberWithTimestampImpl(component, id, time, event_count, nullptr);
}

void LatencyInfo::AddLatencyNumberWithTimestampImpl(
    LatencyComponentType component,
    int64_t id,
    base::TimeTicks time,
    uint32_t event_count,
    const char* trace_name_str) {
  const unsigned char* latency_info_enabled =
      g_latency_info_enabled.Get().latency_info_enabled;

  if (IsTraceBeginComponent(component)) {
    // Should only ever add begin component once.
    CHECK(!began_);
    began_ = true;
    // We should have a trace ID assigned by now.
    DCHECK(trace_id_ != -1);

    if (*latency_info_enabled) {
      // The timestamp for ASYNC_BEGIN trace event is used for drawing the
      // beginning of the trace event in trace viewer. For better visualization,
      // for an input event, we want to draw the beginning as when the event is
      // originally created, e.g. the timestamp of its ORIGINAL/UI_COMPONENT,
      // not when we actually issue the ASYNC_BEGIN trace event.
      LatencyComponent begin_component;
      base::TimeTicks ts;
      if (FindLatency(INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 0,
                      &begin_component) ||
          FindLatency(INPUT_EVENT_LATENCY_UI_COMPONENT, 0, &begin_component)) {
        ts = begin_component.event_time;
      } else {
        ts = base::TimeTicks::Now();
      }

      if (trace_name_str) {
        if (IsInputLatencyBeginComponent(component))
          trace_name_ = std::string("InputLatency::") + trace_name_str;
        else
          trace_name_ = std::string("Latency::") + trace_name_str;
      }

      TRACE_EVENT_COPY_ASYNC_BEGIN_WITH_TIMESTAMP0(
          kTraceCategoriesForAsyncEvents,
          trace_name_.c_str(),
          TRACE_ID_DONT_MANGLE(trace_id_),
          ts);
    }

    TRACE_EVENT_WITH_FLOW1("input,benchmark",
                           "LatencyInfo.Flow",
                           TRACE_ID_DONT_MANGLE(trace_id_),
                           TRACE_EVENT_FLAG_FLOW_OUT,
                           "trace_id", trace_id_);
  }

  LatencyMap::key_type key = std::make_pair(component, id);
  LatencyMap::iterator it = latency_components_.find(key);
  if (it == latency_components_.end()) {
    LatencyComponent info = {time, event_count, time, time};
    latency_components_[key] = info;
  } else {
    uint32_t new_count = event_count + it->second.event_count;
    if (event_count > 0 && new_count != 0) {
      // Do a weighted average, so that the new event_time is the average of
      // the times of events currently in this structure with the time passed
      // into this method.
      it->second.event_time += (time - it->second.event_time) * event_count /
          new_count;
      it->second.event_count = new_count;
      it->second.last_event_time = std::max(it->second.last_event_time, time);
    }
  }

  if (IsTraceEndComponent(component) && began_) {
    // Should only ever add terminal component once.
    CHECK(!terminated_);
    terminated_ = true;

    if (*latency_info_enabled) {
      TRACE_EVENT_COPY_ASYNC_END1(
          kTraceCategoriesForAsyncEvents, trace_name_.c_str(),
          TRACE_ID_DONT_MANGLE(trace_id_), "data", AsTraceableData());
    }

    TRACE_EVENT_WITH_FLOW0("input,benchmark",
                           "LatencyInfo.Flow",
                           TRACE_ID_DONT_MANGLE(trace_id_),
                           TRACE_EVENT_FLAG_FLOW_IN);
  }
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
LatencyInfo::AsTraceableData() {
  std::unique_ptr<base::DictionaryValue> record_data(
      new base::DictionaryValue());
  for (const auto& lc : latency_components_) {
    std::unique_ptr<base::DictionaryValue> component_info(
        new base::DictionaryValue());
    component_info->SetDouble("comp_id", static_cast<double>(lc.first.second));
    component_info->SetDouble(
        "time", static_cast<double>(
                    lc.second.event_time.since_origin().InMicroseconds()));
    component_info->SetDouble("count", lc.second.event_count);
    record_data->Set(GetComponentName(lc.first.first),
                     std::move(component_info));
  }
  record_data->SetDouble("trace_id", static_cast<double>(trace_id_));
  for (const auto& snapshot : snapshots_) {
    std::unique_ptr<base::DictionaryValue> snapshot_info(
        new base::DictionaryValue());
    snapshot_info->SetInteger("frame_id", snapshot.first);
    snapshot_info->SetInteger("snapshot_id", snapshot.second);
    record_data->Set("snapshot_info", std::move(snapshot_info));
  }
  return LatencyInfoTracedValue::FromValue(std::move(record_data));
}

bool LatencyInfo::FindLatency(LatencyComponentType type,
                              int64_t id,
                              LatencyComponent* output) const {
  LatencyMap::const_iterator it = latency_components_.find(
      std::make_pair(type, id));
  if (it == latency_components_.end())
    return false;
  if (output)
    *output = it->second;
  return true;
}

bool LatencyInfo::FindLatency(LatencyComponentType type,
                              LatencyComponent* output) const {
  LatencyMap::const_iterator it = latency_components_.begin();
  while (it != latency_components_.end()) {
    if (it->first.first == type) {
      if (output)
        *output = it->second;
      return true;
    }
    ++it;
  }
  return false;
}

void LatencyInfo::RemoveLatency(LatencyComponentType type) {
  LatencyMap::iterator it = latency_components_.begin();
  while (it != latency_components_.end()) {
    if (it->first.first == type)
      it = latency_components_.erase(it);
    else
      it++;
  }
}

}  // namespace ui
