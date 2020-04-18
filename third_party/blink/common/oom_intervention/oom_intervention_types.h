// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_COMMON_OOM_INTERVENTION_OOM_INTERVENTION_TYPES_H_
#define THIRD_PARTY_BLINK_COMMON_OOM_INTERVENTION_OOM_INTERVENTION_TYPES_H_

#include <stdint.h>

namespace blink {

// The struct with renderer metrics that are used to detect OOMs. This is stored
// in shared memory so that browser can read it even after the renderer dies.
struct OomInterventionMetrics {
  uint64_t current_private_footprint_kb;
  uint64_t current_swap_kb;

  // Stores the total of V8, BlinkGC and PartitionAlloc memory usage.
  uint64_t current_blink_usage_kb;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_COMMON_OOM_INTERVENTION_OOM_INTERVENTION_TYPES_H_
