// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/touch_action_region.h"

#include "ui/gfx/geometry/rect.h"

namespace cc {

TouchActionRegion::TouchActionRegion() : region_(std::make_unique<Region>()) {}
TouchActionRegion::TouchActionRegion(
    const TouchActionRegion& touch_action_region)
    : map_(touch_action_region.map_),
      region_(std::make_unique<Region>(touch_action_region.region())) {}
TouchActionRegion::TouchActionRegion(TouchActionRegion&& touch_action_region) =
    default;

TouchActionRegion::TouchActionRegion(
    const base::flat_map<TouchAction, Region>& region_map)
    : map_(region_map), region_(std::make_unique<Region>()) {
  for (const auto& pair : region_map) {
    region_->Union(pair.second);
  }
}

TouchActionRegion::~TouchActionRegion() = default;

void TouchActionRegion::Union(TouchAction touch_action, const gfx::Rect& rect) {
  region_->Union(rect);
  map_[touch_action].Union(rect);
}

const Region& TouchActionRegion::GetRegionForTouchAction(
    TouchAction touch_action) const {
  static const Region* empty_region = new Region;
  auto it = map_.find(touch_action);
  if (it == map_.end())
    return *empty_region;
  return it->second;
}

TouchAction TouchActionRegion::GetWhiteListedTouchAction(
    const gfx::Point& point) const {
  TouchAction white_listed_touch_action = kTouchActionAuto;
  for (const auto& pair : map_) {
    if (!pair.second.Contains(point))
      continue;
    white_listed_touch_action &= pair.first;
  }
  return white_listed_touch_action;
}

TouchActionRegion& TouchActionRegion::operator=(
    const TouchActionRegion& other) {
  *region_ = *other.region_;
  map_ = other.map_;
  return *this;
}

TouchActionRegion& TouchActionRegion::operator=(TouchActionRegion&& other) =
    default;

bool TouchActionRegion::operator==(const TouchActionRegion& other) const {
  return map_ == other.map_;
}

}  // namespace cc
