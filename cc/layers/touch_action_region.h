// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_TOUCH_ACTION_REGION_H_
#define CC_LAYERS_TOUCH_ACTION_REGION_H_

#include "base/containers/flat_map.h"
#include "cc/base/region.h"
#include "cc/cc_export.h"
#include "cc/input/touch_action.h"

namespace cc {

class Rect;

class CC_EXPORT TouchActionRegion {
 public:
  TouchActionRegion();
  TouchActionRegion(const TouchActionRegion& touch_action_region);
  TouchActionRegion(TouchActionRegion&& touch_action_region);
  explicit TouchActionRegion(const base::flat_map<TouchAction, Region>&);
  ~TouchActionRegion();

  void Union(TouchAction, const gfx::Rect&);
  const Region& region() const { return *region_; }
  const Region& GetRegionForTouchAction(TouchAction) const;

  // Returns the touch actions that we are sure will be allowed at the point
  // by finding the intersection of all touch actions whose regions contain the
  // given point. If the map is empty, |kTouchActionAuto| is returned since no
  // touch actions have been explicitly defined and the default touch action
  // is auto.
  TouchAction GetWhiteListedTouchAction(const gfx::Point&) const;
  TouchActionRegion& operator=(const TouchActionRegion& other);
  TouchActionRegion& operator=(TouchActionRegion&& other);
  bool operator==(const TouchActionRegion& other) const;

 private:
  // TODO(hayleyferr): Region could be owned directly instead of in a
  // unique_ptr if Region supported efficient moving.
  base::flat_map<TouchAction, Region> map_;
  std::unique_ptr<Region> region_;
};

}  // namespace cc

#endif  // CC_LAYERS_TOUCH_ACTION_REGION_H_
