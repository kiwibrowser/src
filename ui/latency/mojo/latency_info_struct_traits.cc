// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/mojo/latency_info_struct_traits.h"

#include "mojo/public/cpp/base/time_mojom_traits.h"

namespace mojo {

namespace {

ui::mojom::LatencyComponentType UILatencyComponentTypeToMojo(
    ui::LatencyComponentType type) {
  switch (type) {
    case ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT;
    case ui::LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT;
    case ui::LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT:
      return ui::mojom::LatencyComponentType::
          LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_UI_COMPONENT:
      return ui::mojom::LatencyComponentType::INPUT_EVENT_LATENCY_UI_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT;
    case ui::TAB_SHOW_COMPONENT:
      return ui::mojom::LatencyComponentType::TAB_SHOW_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT;
    case ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT:
      return ui::mojom::LatencyComponentType::
          DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT;
    case ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL;
    case ui::INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT;
    case ui::INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT:
      return ui::mojom::LatencyComponentType::
          INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT;
  }
  NOTREACHED();
  return ui::mojom::LatencyComponentType::LATENCY_COMPONENT_TYPE_LAST;
}

ui::LatencyComponentType MojoLatencyComponentTypeToUI(
    ui::mojom::LatencyComponentType type) {
  switch (type) {
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT:
      return ui::LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_RENDERER_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_RENDERER_INVALIDATE_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_RENDERER_COMPOSITOR_COMPONENT;
    case ui::mojom::LatencyComponentType::LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_UI_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_UI_COMPOSITOR_COMPONENT;
    case ui::mojom::LatencyComponentType::
        LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT:
      return ui::LATENCY_BEGIN_FRAME_DISPLAY_COMPOSITOR_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT;
    case ui::mojom::LatencyComponentType::INPUT_EVENT_LATENCY_UI_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_UI_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT;
    case ui::mojom::LatencyComponentType::INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT;
    case ui::mojom::LatencyComponentType::TAB_SHOW_COMPONENT:
      return ui::TAB_SHOW_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT;
    case ui::mojom::LatencyComponentType::
        DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT:
      return ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT;
    case ui::mojom::LatencyComponentType::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT:
      return ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL:
      return ui::INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_FAILED_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_TERMINATED_COMMIT_NO_UPDATE_COMPONENT;
    case ui::mojom::LatencyComponentType::
        INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT:
      return ui::INPUT_EVENT_LATENCY_TERMINATED_SWAP_FAILED_COMPONENT;
  }
  NOTREACHED();
  return ui::LATENCY_COMPONENT_TYPE_LAST;
}

ui::mojom::SourceEventType UISourceEventTypeToMojo(ui::SourceEventType type) {
  switch (type) {
    case ui::UNKNOWN:
      return ui::mojom::SourceEventType::UNKNOWN;
    case ui::WHEEL:
      return ui::mojom::SourceEventType::WHEEL;
    case ui::MOUSE:
      return ui::mojom::SourceEventType::MOUSE;
    case ui::TOUCH:
      return ui::mojom::SourceEventType::TOUCH;
    case ui::INERTIAL:
      return ui::mojom::SourceEventType::INERTIAL;
    case ui::KEY_PRESS:
      return ui::mojom::SourceEventType::KEY_PRESS;
    case ui::FRAME:
      return ui::mojom::SourceEventType::FRAME;
    case ui::OTHER:
      return ui::mojom::SourceEventType::OTHER;
  }
  NOTREACHED();
  return ui::mojom::SourceEventType::UNKNOWN;
}

ui::SourceEventType MojoSourceEventTypeToUI(ui::mojom::SourceEventType type) {
  switch (type) {
    case ui::mojom::SourceEventType::UNKNOWN:
      return ui::UNKNOWN;
    case ui::mojom::SourceEventType::WHEEL:
      return ui::WHEEL;
    case ui::mojom::SourceEventType::MOUSE:
      return ui::MOUSE;
    case ui::mojom::SourceEventType::TOUCH:
      return ui::TOUCH;
    case ui::mojom::SourceEventType::INERTIAL:
      return ui::INERTIAL;
    case ui::mojom::SourceEventType::KEY_PRESS:
      return ui::KEY_PRESS;
    case ui::mojom::SourceEventType::FRAME:
      return ui::FRAME;
    case ui::mojom::SourceEventType::OTHER:
      return ui::OTHER;
  }
  NOTREACHED();
  return ui::SourceEventType::UNKNOWN;
}

}  // namespace

// static
base::TimeTicks StructTraits<ui::mojom::LatencyComponentDataView,
                             ui::LatencyInfo::LatencyComponent>::
    event_time(const ui::LatencyInfo::LatencyComponent& component) {
  return component.event_time;
}

// static
uint32_t StructTraits<ui::mojom::LatencyComponentDataView,
                      ui::LatencyInfo::LatencyComponent>::
    event_count(const ui::LatencyInfo::LatencyComponent& component) {
  return component.event_count;
}

// static
base::TimeTicks StructTraits<ui::mojom::LatencyComponentDataView,
                             ui::LatencyInfo::LatencyComponent>::
    first_event_time(const ui::LatencyInfo::LatencyComponent& component) {
  return component.first_event_time;
}

// static
base::TimeTicks StructTraits<ui::mojom::LatencyComponentDataView,
                             ui::LatencyInfo::LatencyComponent>::
    last_event_time(const ui::LatencyInfo::LatencyComponent& component) {
  return component.last_event_time;
}

// static
bool StructTraits<ui::mojom::LatencyComponentDataView,
                  ui::LatencyInfo::LatencyComponent>::
    Read(ui::mojom::LatencyComponentDataView data,
         ui::LatencyInfo::LatencyComponent* out) {
  if (!data.ReadEventTime(&out->event_time))
    return false;
  if (!data.ReadFirstEventTime(&out->first_event_time))
    return false;
  if (!data.ReadLastEventTime(&out->last_event_time))
    return false;
  out->event_count = data.event_count();
  return true;
}

// static
ui::mojom::LatencyComponentType
StructTraits<ui::mojom::LatencyComponentIdDataView,
             std::pair<ui::LatencyComponentType, int64_t>>::
    type(const std::pair<ui::LatencyComponentType, int64_t>& id) {
  return UILatencyComponentTypeToMojo(id.first);
}

// static
int64_t StructTraits<ui::mojom::LatencyComponentIdDataView,
                     std::pair<ui::LatencyComponentType, int64_t>>::
    id(const std::pair<ui::LatencyComponentType, int64_t>& id) {
  return id.second;
}

// static
bool StructTraits<ui::mojom::LatencyComponentIdDataView,
                  std::pair<ui::LatencyComponentType, int64_t>>::
    Read(ui::mojom::LatencyComponentIdDataView data,
         std::pair<ui::LatencyComponentType, int64_t>* out) {
  out->first = MojoLatencyComponentTypeToUI(data.type());
  out->second = data.id();
  return true;
}

// static
const std::string&
StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::trace_name(
    const ui::LatencyInfo& info) {
  return info.trace_name_;
}

// static
const ui::LatencyInfo::LatencyMap&
StructTraits<ui::mojom::LatencyInfoDataView,
             ui::LatencyInfo>::latency_components(const ui::LatencyInfo& info) {
  return info.latency_components();
}

// static
int64_t StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::trace_id(
    const ui::LatencyInfo& info) {
  return info.trace_id();
}

// static
const ui::LatencyInfo::SnapshotMap&
StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::snapshots(
    const ui::LatencyInfo& info) {
  return info.Snapshots();
}

// static
ukm::SourceId
StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::ukm_source_id(
    const ui::LatencyInfo& info) {
  return info.ukm_source_id();
}

// static
bool StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::coalesced(
    const ui::LatencyInfo& info) {
  return info.coalesced();
}

// static
bool StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::began(
    const ui::LatencyInfo& info) {
  return info.began();
}

// static
bool StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::terminated(
    const ui::LatencyInfo& info) {
  return info.terminated();
}

// static
ui::mojom::SourceEventType
StructTraits<ui::mojom::LatencyInfoDataView,
             ui::LatencyInfo>::source_event_type(const ui::LatencyInfo& info) {
  return UISourceEventTypeToMojo(info.source_event_type());
}

// static
bool StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo>::Read(
    ui::mojom::LatencyInfoDataView data,
    ui::LatencyInfo* out) {
  if (!data.ReadTraceName(&out->trace_name_))
    return false;

  mojo::ArrayDataView<ui::mojom::LatencyComponentPairDataView> components;
  data.GetLatencyComponentsDataView(&components);
  for (uint32_t i = 0; i < components.size(); ++i) {
    ui::mojom::LatencyComponentPairDataView component_pair;
    components.GetDataView(i, &component_pair);
    ui::LatencyInfo::LatencyMap::key_type key;
    if (!component_pair.ReadKey(&key))
      return false;
    auto& value = out->latency_components_[key];
    if (!component_pair.ReadValue(&value))
      return false;
  }

  out->trace_id_ = data.trace_id();
  if (!data.ReadSnapshots(&out->snapshots_))
    return false;
  out->ukm_source_id_ = data.ukm_source_id();
  out->coalesced_ = data.coalesced();
  out->began_ = data.began();
  out->terminated_ = data.terminated();
  out->source_event_type_ = MojoSourceEventTypeToUI(data.source_event_type());

  return true;
}

}  // namespace mojo
