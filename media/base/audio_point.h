// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_AUDIO_POINT_H_
#define MEDIA_BASE_AUDIO_POINT_H_

#include <string>
#include <vector>

#include "media/base/media_shmem_export.h"
#include "ui/gfx/geometry/point3_f.h"

namespace media {

using Point = gfx::Point3F;

// Returns a vector of points parsed from a whitespace-separated string
// formatted as: "x1 y1 z1 ... zn yn zn" for n points.
//
// Returns an empty vector if |points_string| is empty or isn't parseable.
MEDIA_SHMEM_EXPORT std::vector<Point> ParsePointsFromString(
    const std::string& points_string);

// Returns |points| as a human-readable string. (Not necessarily in the format
// required by ParsePointsFromString).
MEDIA_SHMEM_EXPORT std::string PointsToString(const std::vector<Point>& points);

}  // namespace media

#endif  // MEDIA_BASE_AUDIO_POINT_H_
