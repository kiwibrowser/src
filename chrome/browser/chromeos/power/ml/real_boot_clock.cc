// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/real_boot_clock.h"

#include <time.h>

#include "base/logging.h"

namespace chromeos {
namespace power {
namespace ml {

RealBootClock::RealBootClock() = default;
RealBootClock::~RealBootClock() = default;

base::TimeDelta RealBootClock::GetTimeSinceBoot() {
  struct timespec ts = {0};
  const int ret = clock_gettime(CLOCK_BOOTTIME, &ts);
  DCHECK_EQ(ret, 0);
  return base::TimeDelta::FromSeconds(ts.tv_sec) +
         base::TimeDelta::FromNanoseconds(ts.tv_nsec);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
