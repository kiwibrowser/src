// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "content/public/common/page_zoom.h"

namespace content {

const double kMinimumZoomFactor = 0.25;
const double kMaximumZoomFactor = 5.0;
const double kEpsilon = 0.001;
const double kTextSizeMultiplierRatio = 1.2;

bool ZoomValuesEqual(double value_a, double value_b) {
  return (std::fabs(value_a - value_b) <= kEpsilon);
}

double ZoomLevelToZoomFactor(double zoom_level) {
  return std::pow(kTextSizeMultiplierRatio, zoom_level);
}

double ZoomFactorToZoomLevel(double factor) {
  return std::log(factor) / std::log(kTextSizeMultiplierRatio);
}

}  // namespace content
