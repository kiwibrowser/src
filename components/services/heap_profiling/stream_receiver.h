// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_RECEIVER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_RECEIVER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace heap_profiling {

// A stream receiver is a sink for unparsed bytes. See also LogReceiver.
class StreamReceiver : public base::RefCountedThreadSafe<StreamReceiver> {
 public:
  StreamReceiver() {}

  // Returns true on success, false on unrecoverable error. The implementation
  // should be able to handle calls after an error has been reported (some
  // cross-thread calls may have been dispatched before the flag propagates).
  virtual bool OnStreamData(std::unique_ptr<char[]> data, size_t sz) = 0;

  // Indicates the connection has been closed.
  virtual void OnStreamComplete() = 0;

 protected:
  friend class base::RefCountedThreadSafe<StreamReceiver>;
  virtual ~StreamReceiver() {}
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_RECEIVER_H_
