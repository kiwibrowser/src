// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_LATENCY_MOJO_LATENCY_INFO_STRUCT_TRAITS_H_
#define UI_LATENCY_MOJO_LATENCY_INFO_STRUCT_TRAITS_H_

#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "ui/latency/latency_info.h"
#include "ui/latency/mojo/latency_info.mojom-shared.h"

namespace mojo {

static_assert(
    static_cast<int>(
        ui::mojom::LatencyComponentType::LATENCY_COMPONENT_TYPE_LAST) ==
        static_cast<int>(ui::LATENCY_COMPONENT_TYPE_LAST),
    "Enum size mismatch");

static_assert(
    static_cast<int>(ui::mojom::SourceEventType::SOURCE_EVENT_TYPE_LAST) ==
        static_cast<int>(ui::SOURCE_EVENT_TYPE_LAST),
    "Enum size mismatch");

template <>
struct ArrayTraits<ui::LatencyInfo::LatencyMap> {
  using Element = ui::LatencyInfo::LatencyMap::value_type;
  using Iterator = ui::LatencyInfo::LatencyMap::iterator;
  using ConstIterator = ui::LatencyInfo::LatencyMap::const_iterator;

  static ConstIterator GetBegin(const ui::LatencyInfo::LatencyMap& input) {
    return input.begin();
  }
  static Iterator GetBegin(ui::LatencyInfo::LatencyMap& input) {
    return input.begin();
  }

  static void AdvanceIterator(ConstIterator& iterator) { iterator++; }
  static void AdvanceIterator(Iterator& iterator) { iterator++; }

  static const Element& GetValue(ConstIterator& iterator) { return *iterator; }
  static Element& GetValue(Iterator& iterator) { return *iterator; }

  static size_t GetSize(const ui::LatencyInfo::LatencyMap& input) {
    return input.size();
  }
};

template <>
struct StructTraits<ui::mojom::LatencyComponentDataView,
                    ui::LatencyInfo::LatencyComponent> {
  static base::TimeTicks event_time(
      const ui::LatencyInfo::LatencyComponent& component);
  static uint32_t event_count(
      const ui::LatencyInfo::LatencyComponent& component);
  static base::TimeTicks first_event_time(
      const ui::LatencyInfo::LatencyComponent& component);
  static base::TimeTicks last_event_time(
      const ui::LatencyInfo::LatencyComponent& component);
  static bool Read(ui::mojom::LatencyComponentDataView data,
                   ui::LatencyInfo::LatencyComponent* out);
};

template <>
struct StructTraits<ui::mojom::LatencyComponentPairDataView,
                    ui::LatencyInfo::LatencyMap::value_type> {
  static const std::pair<ui::LatencyComponentType, int64_t>& key(
      const ui::LatencyInfo::LatencyMap::value_type& input) {
    return input.first;
  }

  static const ui::LatencyInfo::LatencyComponent& value(
      const ui::LatencyInfo::LatencyMap::value_type& input) {
    return input.second;
  }

  // TODO(fsamuel): Figure out how to optimize deserialization.
};

template <>
struct StructTraits<ui::mojom::LatencyComponentIdDataView,
                    std::pair<ui::LatencyComponentType, int64_t>> {
  static ui::mojom::LatencyComponentType type(
      const std::pair<ui::LatencyComponentType, int64_t>& id);
  static int64_t id(const std::pair<ui::LatencyComponentType, int64_t>& id);
  static bool Read(ui::mojom::LatencyComponentIdDataView data,
                   std::pair<ui::LatencyComponentType, int64_t>* out);
};

template <>
struct StructTraits<ui::mojom::LatencyInfoDataView, ui::LatencyInfo> {
  static const std::string& trace_name(const ui::LatencyInfo& info);
  static const ui::LatencyInfo::LatencyMap& latency_components(
      const ui::LatencyInfo& info);
  static int64_t trace_id(const ui::LatencyInfo& info);
  static const ui::LatencyInfo::SnapshotMap& snapshots(
      const ui::LatencyInfo& info);
  static ukm::SourceId ukm_source_id(const ui::LatencyInfo& info);
  static bool coalesced(const ui::LatencyInfo& info);
  static bool began(const ui::LatencyInfo& info);
  static bool terminated(const ui::LatencyInfo& info);
  static ui::mojom::SourceEventType source_event_type(
      const ui::LatencyInfo& info);
  static bool Read(ui::mojom::LatencyInfoDataView data, ui::LatencyInfo* out);
};

}  // namespace mojo

#endif  // UI_LATENCY_MOJO_LATENCY_INFO_STRUCT_TRAITS_H_
