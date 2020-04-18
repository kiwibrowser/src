// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/services/resource_limits.h"
#include "base/numerics/safe_math.h"

#include <sys/resource.h>
#include <sys/time.h>

#include <algorithm>

namespace sandbox {

// static
bool ResourceLimits::Lower(int resource, rlim_t limit) {
  return LowerSoftAndHardLimits(resource, limit, limit);
}

// static
bool ResourceLimits::LowerSoftAndHardLimits(int resource,
                                            rlim_t soft_limit,
                                            rlim_t hard_limit) {
  struct rlimit old_rlimit;
  if (getrlimit(resource, &old_rlimit))
    return false;
  // Make sure we don't raise the existing limit.
  const struct rlimit new_rlimit = {std::min(old_rlimit.rlim_cur, soft_limit),
                                    std::min(old_rlimit.rlim_max, hard_limit)};
  return setrlimit(resource, &new_rlimit) == 0;
}

// static
bool ResourceLimits::AdjustCurrent(int resource, long long int change) {
  struct rlimit old_rlimit;
  if (getrlimit(resource, &old_rlimit))
    return false;
  base::CheckedNumeric<rlim_t> checked_limit = old_rlimit.rlim_cur;
  checked_limit += change;
  const rlim_t new_limit = checked_limit.ValueOrDefault(old_rlimit.rlim_max);
  const struct rlimit new_rlimit = {std::min(new_limit, old_rlimit.rlim_max),
                                    old_rlimit.rlim_max};
  // setrlimit will fail if limit > old_rlimit.rlim_max.
  return setrlimit(resource, &new_rlimit) == 0;
}

}  // namespace sandbox
