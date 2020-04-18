// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_VIEWPORT_METRICS_H_
#define SERVICES_UI_DISPLAY_VIEWPORT_METRICS_H_

#include <string>

#include "ui/gfx/geometry/rect.h"

namespace display {

struct ViewportMetrics {
  std::string ToString() const;

  gfx::Rect bounds_in_pixels;
  float device_scale_factor = 0.0f;
  float ui_scale_factor = 0.0f;
};

inline bool operator==(const ViewportMetrics& lhs, const ViewportMetrics& rhs) {
  return lhs.bounds_in_pixels == rhs.bounds_in_pixels &&
         lhs.device_scale_factor == rhs.device_scale_factor &&
         lhs.ui_scale_factor == rhs.ui_scale_factor;
}

inline bool operator!=(const ViewportMetrics& lhs, const ViewportMetrics& rhs) {
  return !(lhs == rhs);
}

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_VIEWPORT_METRICS_H_
