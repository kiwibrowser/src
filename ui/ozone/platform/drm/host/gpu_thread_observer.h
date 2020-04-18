// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_GPU_THREAD_OBSERVER_H_
#define UI_OZONE_PLATFORM_DRM_HOST_GPU_THREAD_OBSERVER_H_

namespace ui {

// Observes the channel state. All calls should happen on the same thread that
// OzonePlatform::InitializeForUI() is called on. This can be the browser UI
// thread or the WS thread for mus/mash.
class GpuThreadObserver {
 public:
  virtual ~GpuThreadObserver() {}

  // Called when the GPU process is launched.
  virtual void OnGpuProcessLaunched() = 0;
  // Called when a GPU thread implementation has become available.
  virtual void OnGpuThreadReady() = 0;
  // Called when the GPU thread implementation has ceased to be
  // available.
  virtual void OnGpuThreadRetired() = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_GPU_THREAD_OBSERVER_H_
