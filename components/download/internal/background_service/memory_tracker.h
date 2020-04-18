// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_MEMORY_TRACKER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_MEMORY_TRACKER_H_

#include <stddef.h>

namespace download {

// Used to track memory usage in download service for pure virtual interfaces.
class MemoryTracker {
 public:
  virtual ~MemoryTracker() = default;

  // Returns the estimate of dynamically allocated memory in bytes.
  virtual size_t EstimateMemoryUsage() const = 0;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_MEMORY_TRACKER_H_
