// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/viewport_metrics.h"

#include "base/strings/stringprintf.h"

namespace display {

std::string ViewportMetrics::ToString() const {
  return base::StringPrintf(
      "ViewportMetrics(bounds_in_pixels=%s, device_scale_factor=%g, "
      "ui_scale_factor=%g)",
      bounds_in_pixels.ToString().c_str(), device_scale_factor,
      ui_scale_factor);
}

}  // namespace display
