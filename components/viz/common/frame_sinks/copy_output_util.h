// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_FRAME_SINKS_COPY_OUTPUT_UTIL_H_
#define COMPONENTS_VIZ_COMMON_FRAME_SINKS_COPY_OUTPUT_UTIL_H_

#include "components/viz/common/viz_common_export.h"

namespace gfx {
class Rect;
class Vector2d;
}  // namespace gfx

namespace viz {
namespace copy_output {

// Returns the pixels in the scaled result coordinate space that are affected by
// the source |area| and scaling ratio. If application of the scaling ratio
// generates coordinates that are out-of-range or otherwise not "safely
// reasonable," an empty Rect is returned.
gfx::Rect VIZ_COMMON_EXPORT ComputeResultRect(const gfx::Rect& area,
                                              const gfx::Vector2d& scale_from,
                                              const gfx::Vector2d& scale_to);

}  // namespace copy_output
}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_FRAME_SINKS_COPY_OUTPUT_UTIL_H_
