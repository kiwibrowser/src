// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_LATENCY_LATENCY_INFO_H_
#define UI_LATENCY_LATENCY_INFO_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "ui/gfx/geometry/point_f.h"

#if !defined(OS_IOS)
#include "ipc/ipc_param_traits.h"  // nogncheck
#include "mojo/public/cpp/bindings/struct_traits.h"  // nogncheck
#endif

namespace base {
namespace trace_event {
class ConvertableToTraceFormat;
}
}

namespace ui {

#if !defined(OS_IOS)
namespace mojom {
class LatencyInfoDataView;
}
#endif

// When adding new components, or new metrics based on LatencyInfo,
// please update latency_info.dot.
enum LatencyComponentType {
  // ---------------------------BEGIN COMPONENT-------------------------------
  // BEGIN COMPONENT is when we show the latency begin in chrome://tracing.
  // Timestamp when the input event is sent from RenderWidgetHost to renderer.
  INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT,
  // In threaded scrolling, main thread scroll listener update is async to
  // scroll processing in impl thread. This is the timestamp when we consider
  // the main thread scroll listener update is begun.
  LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT,
  // The BeginFrame::frame_time of various frame sources.
  LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT,
  LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT,
  LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT,
  LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT,
  LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT,
  LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT,
  // ---------------------------NORMAL COMPONENT-------------------------------
  // The original timestamp of the touch event which converts to scroll update.
  INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT,
  // The original timestamp of the touch event which converts to the *first*
  // scroll update in a scroll gesture sequence.
  INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT,
  // Original timestamp for input event (e.g. timestamp from kernel).
  INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT,
  // Timestamp when the UI event is created.
  INPUT_EVENT_LATENCY_UI_COMPONENT,
  // Timestamp when the event is dispatched on the main thread of the renderer.
  INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT,
  // This is special component indicating there is rendering scheduled for
  // the event associated with this LatencyInfo on main thread.
  INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT,
  // This is special component indicating there is rendering scheduled for
  // the event associated with this LatencyInfo on impl thread.
  INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT,
  // Timestamp when a scroll update is forwarded to the main thread.
  INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT,
  // Timestamp when the event's ack is received by the RWH.
  INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT,
  // Timestamp when a tab is requested to be shown.
  TAB_SHOW_COMPONENT,
  // Timestamp when the frame is swapped in renderer.
  INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT,
  // Timestamp of when the display compositor receives a compositor frame from
  // the renderer.
  // Display compositor can be either in the browser process or in Mus.
  DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT,
  // Timestamp of when the gpu service began swap buffers, unlike
  // INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT which measures after.
  INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT,
  // Timestamp of when the gesture scroll update is generated from a mouse wheel
  // event.
  INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL,
  // ---------------------------TERMINAL COMPONENT-----------------------------
  // Timestamp when the event is acked from renderer when it does not
  // cause any rendering to be scheduled.
  INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT,
  // Timestamp when the frame is swapped (i.e. when the rendering caused by
  // input event actually takes effect).
  INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT,
  // This component indicates that the input causes a commit to be scheduled
  // but the commit failed.
  INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT,
  // This component indicates that the input causes a commit to be scheduled
  // but the commit was aborted since it carried no new information.
  INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT,
  // This component indicates that the input causes a swap to be scheduled
  // but the swap failed.
  INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT,
  LATENCY_COMPONENT_TYPE_LAST =
      INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT,
};

enum SourceEventType {
  UNKNOWN,
  WHEEL,
  MOUSE,
  TOUCH,
  INERTIAL,
  KEY_PRESS,
  FRAME,
  OTHER,
  SOURCE_EVENT_TYPE_LAST = OTHER,
};

class LatencyInfo {
 public:
  struct LatencyComponent {
    // Average time of events that happened in this component.
    base::TimeTicks event_time;
    // Count of events that happened in this component
    uint32_t event_count;
    // Time of the oldest event that happened in this component.
    base::TimeTicks first_event_time;
    // Time of the most recent event that happened in this component.
    base::TimeTicks last_event_time;
  };

  enum : size_t { kMaxInputCoordinates = 2 };

  // Map a Latency Component (with a component-specific int64_t id) to a
  // component info.
  using LatencyMap = base::flat_map<std::pair<LatencyComponentType, int64_t>,
                                    LatencyComponent>;

  // Map a frame sink id to the snapshot id.
  using SnapshotMap = std::map<int64_t, int64_t>;

  LatencyInfo();
  LatencyInfo(const LatencyInfo& other);
  LatencyInfo(SourceEventType type);
  ~LatencyInfo();

  // For test only.
  LatencyInfo(int64_t trace_id, bool terminated);

  // Returns true if the vector |latency_info| is valid. Returns false
  // if it is not valid and log the |referring_msg|.
  // This function is mainly used to check the latency_info vector that
  // is passed between processes using IPC message has reasonable size
  // so that we are confident the IPC message is not corrupted/compromised.
  // This check will go away once the IPC system has better built-in scheme
  // for corruption/compromise detection.
  static bool Verify(const std::vector<LatencyInfo>& latency_info,
                     const char* referring_msg);

  // Adds trace flow events only to LatencyInfos that are being traced.
  static void TraceIntermediateFlowEvents(
      const std::vector<LatencyInfo>& latency_info,
      const char* trace_name);

  // Copy LatencyComponents with type |type| from |other| into |this|.
  void CopyLatencyFrom(const LatencyInfo& other, LatencyComponentType type);

  // Add LatencyComponents that are in |other| but not in |this|.
  void AddNewLatencyFrom(const LatencyInfo& other);

  // Modifies the current sequence number for a component, and adds a new
  // sequence number with the current timestamp.
  void AddLatencyNumber(LatencyComponentType component, int64_t id);

  // Similar to |AddLatencyNumber|, and also appends |trace_name_str| to
  // the trace event's name.
  // This function should only be called when adding a BEGIN component.
  void AddLatencyNumberWithTraceName(LatencyComponentType component,
                                     int64_t id,
                                     const char* trace_name_str);

  // Modifies the current sequence number and adds a certain number of events
  // for a specific component.
  void AddLatencyNumberWithTimestamp(LatencyComponentType component,
                                     int64_t id,
                                     base::TimeTicks time,
                                     uint32_t event_count);

  // Returns true if the a component with |type| and |id| is found in
  // the latency_components and the component is stored to |output| if
  // |output| is not NULL. Returns false if no such component is found.
  bool FindLatency(LatencyComponentType type,
                   int64_t id,
                   LatencyComponent* output) const;

  // Returns true if a component with |type| is found in the latency component.
  // The first such component (when iterating over latency_components_) is
  // stored to |output| if |output| is not NULL. Returns false if no such
  // component is found.
  bool FindLatency(LatencyComponentType type, LatencyComponent* output) const;

  void RemoveLatency(LatencyComponentType type);

  const LatencyMap& latency_components() const { return latency_components_; }

  const SourceEventType& source_event_type() const {
    return source_event_type_;
  }
  void set_source_event_type(SourceEventType type) {
    source_event_type_ = type;
  }

  void AddSnapshot(int64_t frame_sink_id, int64_t snapshot_id) {
    snapshots_[frame_sink_id] = snapshot_id;
  }
  const SnapshotMap& Snapshots() const { return snapshots_; }
  void RemoveSnapshots() { snapshots_.clear(); }
  bool began() const { return began_; }
  bool terminated() const { return terminated_; }
  void set_coalesced() { coalesced_ = true; }
  bool coalesced() const { return coalesced_; }
  int64_t trace_id() const { return trace_id_; }
  void set_trace_id(int64_t trace_id) { trace_id_ = trace_id; }
  ukm::SourceId ukm_source_id() const { return ukm_source_id_; }
  void set_ukm_source_id(ukm::SourceId id) { ukm_source_id_ = id; }
  const std::string& trace_name() const { return trace_name_; }

 private:
  void AddLatencyNumberWithTimestampImpl(LatencyComponentType component,
                                         int64_t id,
                                         base::TimeTicks time,
                                         uint32_t event_count,
                                         const char* trace_name_str);

  // Converts latencyinfo into format that can be dumped into trace buffer.
  std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
  AsTraceableData();

  // Shown as part of the name of the trace event for this LatencyInfo.
  // String is empty if no tracing is enabled.
  std::string trace_name_;

  LatencyMap latency_components_;

  // The unique id for matching the ASYNC_BEGIN/END trace event.
  int64_t trace_id_;
  // Snapshot ids to be used to sync snapshot requests.
  SnapshotMap snapshots_;
  // UKM Source id to be used for recording UKM metrics associated with this
  // event.
  ukm::SourceId ukm_source_id_;
  // Whether this event has been coalesced into another event.
  bool coalesced_;
  // Whether a begin component has been added.
  bool began_;
  // Whether a terminal component has been added.
  bool terminated_;
  // Stores the type of the first source event.
  SourceEventType source_event_type_;

#if !defined(OS_IOS)
  friend struct IPC::ParamTraits<ui::LatencyInfo>;
  friend struct mojo::StructTraits<ui::mojom::LatencyInfoDataView,
                                   ui::LatencyInfo>;
#endif
};

// This is declared here for use in gtest-based unit tests, but is defined in
// //ui/latency:test_support target.
// Without this the default PrintTo template in gtest tries to pass LatencyInfo
// by value, which leads to an alignment compile error on Windows.
void PrintTo(const LatencyInfo& latency, ::std::ostream* os);

}  // namespace ui

#endif  // UI_LATENCY_LATENCY_INFO_H_
