// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIMESTAMP_CONSTANTS_H_
#define MEDIA_BASE_TIMESTAMP_CONSTANTS_H_

#include <stdint.h>

#include <limits>

#include "base/time/time.h"

namespace media {

// Indicates an invalid or missing timestamp.
constexpr base::TimeDelta kNoTimestamp =
    base::TimeDelta::FromMicroseconds(std::numeric_limits<int64_t>::min());

// Represents an infinite stream duration.
constexpr base::TimeDelta kInfiniteDuration =
    base::TimeDelta::FromMicroseconds(std::numeric_limits<int64_t>::max());

}  // namespace media

#endif  // MEDIA_BASE_TIMESTAMP_CONSTANTS_H_
