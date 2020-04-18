// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MAGNIFIER_MAGNIFIER_SCALE_UTILS_H_
#define ASH_MAGNIFIER_MAGNIFIER_SCALE_UTILS_H_

#include "ash/ash_export.h"

namespace ash {
namespace magnifier_scale_utils {

// Factor of magnification scale. For example, when this value is 1.189, scale
// value will be changed x1.000, x1.189, x1.414, x1.681, x2.000, ...
// Note: this value is 2.0 ^ (1 / 4).
constexpr float kMagnificationScaleFactor = 1.18920712f;

// Calculates the new scale if it were to be adjusted exponentially by the
// given |linear_offset|. This allows linear changes in scroll offset
// to have exponential changes on the scale, so that as the user zooms in,
// they appear to zoom faster through higher resolutions. This also has the
// effect that whether the user moves their fingers quickly or slowly on
// the trackpad (changing the number of events fired), the resulting zoom
// will only depend on the distance their fingers traveled.
// |linear_offset| should generally be between 0 and 1, to result in a set
// scale between |min_scale| and |max_scale|.
// The resulting scale should be an exponential of the form
// y = M * x ^ 2 + c, where y is the resulting scale, M is the scale range which
// is the difference between |max_scale| and |min_scale|, and c is the
// |min_scale|. This creates a mapping from |linear_offset| in (0, 1) to a scale
// in [min_scale, max_scale].
float ASH_EXPORT GetScaleFromScroll(float linear_offset,
                                    float current_scale,
                                    float min_scale,
                                    float max_scale);

// Converts the |current_range| to an integral index such that
// `current_scale = kMagnificationScaleFactor ^ index`, increments it by
// |delta_index| and converts it back to a scale value in the range between
// |min_scale| and |max_scale|.
float ASH_EXPORT GetNextMagnifierScaleValue(int delta_index,
                                            float current_scale,
                                            float min_scale,
                                            float max_scale);

}  // namespace magnifier_scale_utils
}  // namespace ash

#endif  // ASH_MAGNIFIER_MAGNIFIER_SCALE_UTILS_H_
