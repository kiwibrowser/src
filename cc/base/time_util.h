// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_TIME_UTIL_H_
#define CC_BASE_TIME_UTIL_H_

#include "base/time/time.h"

namespace cc {

class TimeUtil {
 public:
  static double Divide(base::TimeDelta dividend, base::TimeDelta divisor) {
    return static_cast<double>(dividend.InMicroseconds()) /
           static_cast<double>(divisor.InMicroseconds());
  }
};

}  // namespace cc

#endif  // CC_BASE_TIME_UTIL_H_
