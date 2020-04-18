// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_DOCUMENT_LOAD_STATISTICS_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_DOCUMENT_LOAD_STATISTICS_H_

#include <stdint.h>

#include "base/time/time.h"

namespace subresource_filter {

// Contains statistics and performance metrics collected by the
// DocumentSubresourceFilter.
struct DocumentLoadStatistics {
  // The number of subresource loads that went through the
  // DocumentSubresouceFilter filtering methods.
  int32_t num_loads_total = 0;

  // Statistics on the number of subresource loads that were evaluated, were
  // matched by filtering rules, and were disallowed, respectively, during the
  // lifetime of a DocumentSubresourceFilter.
  int32_t num_loads_evaluated = 0;
  int32_t num_loads_matching_rules = 0;
  int32_t num_loads_disallowed = 0;

  // Total time spent in GetLoadPolicy() calls evaluating subresource loads.
  base::TimeDelta evaluation_total_wall_duration;
  base::TimeDelta evaluation_total_cpu_duration;
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_DOCUMENT_LOAD_STATISTICS_H_
