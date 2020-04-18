// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "components/services/heap_profiling/address.h"
#include "components/services/heap_profiling/public/cpp/stream.h"

namespace heap_profiling {

// A log receiver is a sink for parsed allocation events. See also
// StreamReceiver which is for the unparsed data blocks.
class Receiver {
 public:
  virtual ~Receiver() {}

  virtual void OnHeader(const StreamHeader& header) = 0;
  virtual void OnAlloc(const AllocPacket& alloc_packet,
                       std::vector<Address>&& stack,
                       std::string&& context) = 0;
  virtual void OnFree(const FreePacket& free_packet) = 0;
  virtual void OnBarrier(const BarrierPacket& barrier_packet) = 0;
  virtual void OnStringMapping(const StringMappingPacket& string_mapping_packet,
                               const std::string& str) = 0;
  virtual void OnComplete() = 0;
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_H_
