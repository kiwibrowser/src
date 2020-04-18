// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_ANDROID_CRONET_METRICS_UTIL_H_
#define COMPONENTS_CRONET_ANDROID_CRONET_METRICS_UTIL_H_

#include <stdint.h>

#include "base/time/time.h"

namespace cronet {

namespace metrics_util {

// Converts timing metrics stored as TimeTicks into the format expected by the
// Java layer: ms since Unix epoch, or -1 for null
int64_t ConvertTime(const base::TimeTicks& ticks,
                    const base::TimeTicks& start_ticks,
                    const base::Time& start_time);
}  // namespace metrics_util

}  // namespace cronet

#endif  // COMPONENTS_CRONET_ANDROID_CRONET_METRICS_UTIL_H_
