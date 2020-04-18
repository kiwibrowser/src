// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GPU_DATA_MANAGER_H_
#define CONTENT_PUBLIC_BROWSER_GPU_DATA_MANAGER_H_

#include <list>
#include <string>

#include "base/callback_forward.h"
#include "base/process/process.h"
#include "content/common/content_export.h"

namespace gpu {
struct GPUInfo;
struct VideoMemoryUsageStats;
}

namespace content {

class GpuDataManagerObserver;

// This class is fully thread-safe.
class GpuDataManager {
 public:
  // Getter for the singleton.
  CONTENT_EXPORT static GpuDataManager* GetInstance();

  // This is only called by extensions testing.
  virtual void BlacklistWebGLForTesting() = 0;

  virtual gpu::GPUInfo GetGPUInfo() const = 0;

  // This indicator might change because we could collect more GPU info or
  // because the GPU blacklist could be updated.
  // If this returns false, any further GPU access, including establishing GPU
  // channel, and GPU info collection, should be blocked.
  // Can be called on any thread.
  // If |reason| is not nullptr and GPU access is blocked, upon return, |reason|
  // contains a description of the reason why GPU access is blocked.
  virtual bool GpuAccessAllowed(std::string* reason) const = 0;

  // Requests complete GPU info if it has not already been requested
  virtual void RequestCompleteGpuInfoIfNeeded() = 0;

  // Check if basic and context GPU info have been collected.
  virtual bool IsEssentialGpuInfoAvailable() const = 0;

  // Requests that the GPU process report its current video memory usage stats.
  virtual void RequestVideoMemoryUsageStatsUpdate(
      const base::Callback<void(const gpu::VideoMemoryUsageStats& stats)>&
          callback) const = 0;

  // Registers/unregister |observer|.
  virtual void AddObserver(GpuDataManagerObserver* observer) = 0;
  virtual void RemoveObserver(GpuDataManagerObserver* observer) = 0;

  virtual void DisableHardwareAcceleration() = 0;

  // Whether a GPU is in use (as opposed to a software renderer).
  virtual bool HardwareAccelerationEnabled() const = 0;

 protected:
  virtual ~GpuDataManager() {}
};

};  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GPU_DATA_MANAGER_H_
