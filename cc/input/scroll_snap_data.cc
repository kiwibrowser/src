// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_snap_data.h"

#include <cmath>
#include "base/optional.h"

namespace cc {
namespace {

bool IsVisible(const gfx::ScrollOffset& point,
               const gfx::RectF& visible_region) {
  return point.x() >= visible_region.x() &&
         point.x() <= visible_region.right() &&
         point.y() >= visible_region.y() &&
         point.y() <= visible_region.bottom();
}

bool IsMutualVisible(const SnapAreaData& area_x, const SnapAreaData& area_y) {
  gfx::ScrollOffset position(area_x.snap_position.x(),
                             area_y.snap_position.y());
  return IsVisible(position, area_x.visible_region) &&
         IsVisible(position, area_y.visible_region);
}

bool SnappableOnAxis(const SnapAreaData& area, SearchAxis search_axis) {
  return search_axis == SearchAxis::kX ? area.snap_axis == SnapAxis::kX ||
                                             area.snap_axis == SnapAxis::kBoth
                                       : area.snap_axis == SnapAxis::kY ||
                                             area.snap_axis == SnapAxis::kBoth;
}

// Finds the best SnapArea candidate that minimizes the distance between current
// and candidate positions, while satisfying three invariants:
// - |candidate_position| is in |target_region|
// - |current_position|   is in candidate's visible region
// - |target_position|    is in candidate's visible region

// |current_position| is the scroll position of the container before snapping.
// |target_position| is the snap position we have found on the other axis.
// |target_region| is the visible region of the target position's area.
base::Optional<SnapAreaData> FindClosestValidArea(
    SearchAxis search_axis,
    const gfx::ScrollOffset& current_position,
    const gfx::ScrollOffset& target_position,
    const gfx::RectF& target_region,
    const gfx::ScrollOffset& proximity_range,
    const SnapAreaList& list) {
  if (list.empty())
    return base::nullopt;

  base::Optional<SnapAreaData> closest_area;
  float smallest_distance =
      search_axis == SearchAxis::kX ? proximity_range.x() : proximity_range.y();
  for (const SnapAreaData& area : list) {
    if (!SnappableOnAxis(area, search_axis))
      continue;

    gfx::ScrollOffset candidate_position =
        search_axis == SearchAxis::kX
            ? gfx::ScrollOffset(area.snap_position.x(), target_position.y())
            : gfx::ScrollOffset(target_position.x(), area.snap_position.y());
    if (!IsVisible(candidate_position, target_region) ||
        !IsVisible(current_position, area.visible_region) ||
        !IsVisible(target_position, area.visible_region))
      continue;

    gfx::ScrollOffset offset = current_position - candidate_position;
    float distance = search_axis == SearchAxis::kX ? std::abs(offset.x())
                                                   : std::abs(offset.y());
    if (distance < smallest_distance) {
      smallest_distance = distance;
      closest_area = area;
    }
  }
  return closest_area;
}

}  // namespace

SnapContainerData::SnapContainerData()
    : proximity_range_(gfx::ScrollOffset(std::numeric_limits<float>::max(),
                                         std::numeric_limits<float>::max())) {}

SnapContainerData::SnapContainerData(ScrollSnapType type)
    : scroll_snap_type_(type),
      proximity_range_(gfx::ScrollOffset(std::numeric_limits<float>::max(),
                                         std::numeric_limits<float>::max())) {}

SnapContainerData::SnapContainerData(ScrollSnapType type, gfx::ScrollOffset max)
    : scroll_snap_type_(type),
      max_position_(max),
      proximity_range_(gfx::ScrollOffset(std::numeric_limits<float>::max(),
                                         std::numeric_limits<float>::max())) {}

SnapContainerData::SnapContainerData(const SnapContainerData& other) = default;

SnapContainerData::SnapContainerData(SnapContainerData&& other)
    : scroll_snap_type_(other.scroll_snap_type_),
      max_position_(other.max_position_),
      proximity_range_(other.proximity_range_),
      snap_area_list_(std::move(other.snap_area_list_)) {}

SnapContainerData::~SnapContainerData() = default;

SnapContainerData& SnapContainerData::operator=(
    const SnapContainerData& other) = default;

SnapContainerData& SnapContainerData::operator=(SnapContainerData&& other) {
  scroll_snap_type_ = other.scroll_snap_type_;
  max_position_ = other.max_position_;
  proximity_range_ = other.proximity_range_;
  snap_area_list_ = std::move(other.snap_area_list_);
  return *this;
}

void SnapContainerData::AddSnapAreaData(SnapAreaData snap_area_data) {
  snap_area_list_.push_back(snap_area_data);
}

bool SnapContainerData::FindSnapPosition(
    const gfx::ScrollOffset& current_position,
    bool should_snap_on_x,
    bool should_snap_on_y,
    gfx::ScrollOffset* snap_position) const {
  SnapAxis axis = scroll_snap_type_.axis;
  should_snap_on_x &= (axis == SnapAxis::kX || axis == SnapAxis::kBoth);
  should_snap_on_y &= (axis == SnapAxis::kY || axis == SnapAxis::kBoth);
  if (!should_snap_on_x && !should_snap_on_y)
    return false;

  base::Optional<SnapAreaData> closest_x, closest_y;
  // A region that includes every reachable scroll position.
  gfx::RectF scrollable_region(0, 0, max_position_.x(), max_position_.y());
  if (should_snap_on_x) {
    closest_x = FindClosestValidArea(SearchAxis::kX, current_position,
                                     current_position, scrollable_region,
                                     proximity_range_, snap_area_list_);
  }
  if (should_snap_on_y) {
    closest_y = FindClosestValidArea(SearchAxis::kY, current_position,
                                     current_position, scrollable_region,
                                     proximity_range_, snap_area_list_);
  }

  if (!closest_x.has_value() && !closest_y.has_value())
    return false;

  // If snapping in one axis pushes off-screen the other snap area, this snap
  // position is invalid. https://drafts.csswg.org/css-scroll-snap-1/#snap-scope
  // In this case, we choose the axis whose snap area is closer, and find a
  // mutual visible snap area on the other axis.
  if (closest_x.has_value() && closest_y.has_value() &&
      !IsMutualVisible(closest_x.value(), closest_y.value())) {
    bool candidate_on_x_axis_is_closer =
        std::abs(closest_x.value().snap_position.x() - current_position.x()) <=
        std::abs(closest_y.value().snap_position.y() - current_position.y());
    if (candidate_on_x_axis_is_closer) {
      gfx::ScrollOffset snapped(closest_x.value().snap_position.x(),
                                current_position.y());
      closest_y = FindClosestValidArea(
          SearchAxis::kY, current_position, snapped,
          closest_x.value().visible_region, proximity_range_, snap_area_list_);
    } else {
      gfx::ScrollOffset snapped(current_position.x(),
                                closest_y.value().snap_position.y());
      closest_x = FindClosestValidArea(
          SearchAxis::kX, current_position, snapped,
          closest_y.value().visible_region, proximity_range_, snap_area_list_);
    }
  }

  *snap_position = current_position;
  if (closest_x.has_value())
    snap_position->set_x(closest_x.value().snap_position.x());
  if (closest_y.has_value())
    snap_position->set_y(closest_y.value().snap_position.y());

  return true;
}

std::ostream& operator<<(std::ostream& ostream, const SnapAreaData& area_data) {
  return ostream << area_data.snap_position.ToString() << "\t"
                 << "visible in: " << area_data.visible_region.ToString();
}

std::ostream& operator<<(std::ostream& ostream,
                         const SnapContainerData& container_data) {
  for (size_t i = 0; i < container_data.size(); ++i) {
    ostream << container_data.at(i) << "\n";
  }
  return ostream;
}

}  // namespace cc
