// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/allocation_event.h"

namespace heap_profiling {

AllocationEvent::AllocationEvent(AllocatorType allocator,
                                 Address addr,
                                 size_t sz,
                                 const Backtrace* bt,
                                 int context_id)
    : allocator_(allocator),
      address_(addr),
      size_(sz),
      backtrace_(bt),
      context_id_(context_id) {}

AllocationEvent::AllocationEvent(Address addr) : address_(addr) {}

AllocationCountMap AllocationEventSetToCountMap(const AllocationEventSet& set) {
  AllocationCountMap map;
  for (const auto& alloc : set)
    map[alloc]++;
  return map;
}

}  // namespace heap_profiling
