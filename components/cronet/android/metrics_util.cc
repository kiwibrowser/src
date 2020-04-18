// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/android/metrics_util.h"

#include "base/logging.h"

namespace cronet {

namespace metrics_util {

int64_t ConvertTime(const base::TimeTicks& ticks,
                    const base::TimeTicks& start_ticks,
                    const base::Time& start_time) {
  if (ticks.is_null() || start_ticks.is_null()) {
    return -1;
  }
  DCHECK(!start_time.is_null());
  return (start_time + (ticks - start_ticks)).ToJavaTime();
}

}  // namespace metrics_util

}  // namespace cronet
