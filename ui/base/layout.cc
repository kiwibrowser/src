// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/layout.h"

#include <stddef.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "ui/base/touch/touch_device.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/image/image_skia.h"

namespace ui {

namespace {

std::vector<ScaleFactor>* g_supported_scale_factors = NULL;

}  // namespace

void SetSupportedScaleFactors(
    const std::vector<ui::ScaleFactor>& scale_factors) {
  if (g_supported_scale_factors != NULL)
    delete g_supported_scale_factors;

  g_supported_scale_factors = new std::vector<ScaleFactor>(scale_factors);
  std::sort(g_supported_scale_factors->begin(),
            g_supported_scale_factors->end(),
            [](ScaleFactor lhs, ScaleFactor rhs) {
    return GetScaleForScaleFactor(lhs) < GetScaleForScaleFactor(rhs);
  });

  // Set ImageSkia's supported scales.
  std::vector<float> scales;
  for (std::vector<ScaleFactor>::const_iterator it =
          g_supported_scale_factors->begin();
       it != g_supported_scale_factors->end(); ++it) {
    scales.push_back(GetScaleForScaleFactor(*it));
  }
  gfx::ImageSkia::SetSupportedScales(scales);
}

const std::vector<ScaleFactor>& GetSupportedScaleFactors() {
  DCHECK(g_supported_scale_factors != NULL);
  return *g_supported_scale_factors;
}

ScaleFactor GetSupportedScaleFactor(float scale) {
  DCHECK(g_supported_scale_factors != NULL);
  ScaleFactor closest_match = SCALE_FACTOR_100P;
  float smallest_diff =  std::numeric_limits<float>::max();
  for (size_t i = 0; i < g_supported_scale_factors->size(); ++i) {
    ScaleFactor scale_factor = (*g_supported_scale_factors)[i];
    float diff = std::abs(GetScaleForScaleFactor(scale_factor) - scale);
    if (diff < smallest_diff) {
      closest_match = scale_factor;
      smallest_diff = diff;
    }
  }
  DCHECK_NE(closest_match, SCALE_FACTOR_NONE);
  return closest_match;
}

bool IsSupportedScale(float scale) {
  for (auto scale_factor_idx : *g_supported_scale_factors) {
    if (GetScaleForScaleFactor(scale_factor_idx) == scale)
      return true;
  }
  return false;
}

namespace test {

ScopedSetSupportedScaleFactors::ScopedSetSupportedScaleFactors(
    const std::vector<ui::ScaleFactor>& new_scale_factors) {
  if (g_supported_scale_factors) {
    original_scale_factors_ =
        new std::vector<ScaleFactor>(*g_supported_scale_factors);
  } else {
    original_scale_factors_ = NULL;
  }
  SetSupportedScaleFactors(new_scale_factors);
}

ScopedSetSupportedScaleFactors::~ScopedSetSupportedScaleFactors() {
  if (original_scale_factors_) {
    SetSupportedScaleFactors(*original_scale_factors_);
    delete original_scale_factors_;
  } else {
    delete g_supported_scale_factors;
    g_supported_scale_factors = NULL;
  }
}

}  // namespace test

float GetScaleFactorForNativeView(gfx::NativeView view) {
  // A number of unit tests do not setup the screen.
  if (!display::Screen::GetScreen())
    return 1.0f;
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(view);
  DCHECK(display.is_valid());
  return display.device_scale_factor();
}

}  // namespace ui
