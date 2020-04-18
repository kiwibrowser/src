// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_EVENT_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_EVENT_H_

#include <functional>
#include <map>
#include <unordered_set>

#include "components/services/heap_profiling/address.h"
#include "components/services/heap_profiling/backtrace_storage.h"
#include "components/services/heap_profiling/public/cpp/stream.h"

namespace heap_profiling {

// This class is copyable and assignable.
//
// AllocationEvents can be uniquely identified by their address. Caveat: This is
// true at any given point in time, since each address can only be used in a
// single allocation. However, it's possible that comparing different points in
// time, there are different AllocationEvents with the same address.
class AllocationEvent {
 public:
  // There must be a reference to this kept in the BacktraceStorage object on
  // behalf of this class.
  AllocationEvent(AllocatorType allocator,
                  Address addr,
                  size_t sz,
                  const Backtrace* bt,
                  int context_id);

  // This partial initializer creates an allocation of empty size for
  // searching purposes.
  explicit AllocationEvent(Address addr);

  AllocatorType allocator() const { return allocator_; }

  Address address() const { return address_; }
  size_t size() const { return size_; }

  // The backtrace for this allocation. There must be a reference to this kept
  // in the BacktraceStorage object on behalf of this class.
  const Backtrace* backtrace() const { return backtrace_; }

  // ID into context map, 0 means no context.
  int context_id() const { return context_id_; }

  struct HashByAddress {
    size_t operator()(const AllocationEvent& event) const {
      std::hash<Address> hasher;
      return hasher(event.address());
    }
  };

  struct EqualityByAddress {
    bool operator()(const AllocationEvent& lhs,
                    const AllocationEvent& rhs) const {
      return lhs.address().value == rhs.address().value;
    }
  };

  // Implements < for AllocationEvent using everything but the address.
  struct MetadataPartialLess {
    bool operator()(const AllocationEvent& lhs,
                    const AllocationEvent& rhs) const {
      // Note that we're using pointer compiarisons on the backtrace objects
      // since they're atoms and the actual ordering is not important.
      return std::tie(lhs.size_, lhs.backtrace_, lhs.context_id_,
                      lhs.allocator_) < std::tie(rhs.size_, rhs.backtrace_,
                                                 rhs.context_id_,
                                                 rhs.allocator_);
    }
  };

 private:
  AllocatorType allocator_ = AllocatorType::kMalloc;
  Address address_;
  size_t size_ = 0;
  const Backtrace* backtrace_ = nullptr;
  int context_id_ = 0;
};

// Unique set based on addresses of allocations.
using AllocationEventSet =
    std::unordered_set<AllocationEvent,
                       AllocationEvent::HashByAddress,
                       AllocationEvent::EqualityByAddress>;

// Maps allocation metadata to allocation counts of that type. In this case,
// the address of the AllocationEvent is unused.
using AllocationCountMap =
    std::map<AllocationEvent, int, AllocationEvent::MetadataPartialLess>;

// Aggregates the allocation events to a count map. The address of the
// allocation event in the returned map will be the address of the first item
// in the set with that metadata.
AllocationCountMap AllocationEventSetToCountMap(const AllocationEventSet& set);

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_EVENT_H_
