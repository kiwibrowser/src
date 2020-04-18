// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_THREAD_PROXY_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_THREAD_PROXY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ui/ozone/platform/drm/gpu/drm_thread.h"
#include "ui/ozone/public/interfaces/device_cursor.mojom.h"

namespace ui {

class DrmWindowProxy;
class InterThreadMessagingProxy;

// Mediates the communication between GPU main/IO threads and the DRM thread. It
// serves proxy objects that are safe to call on the GPU threads. The proxy
// objects then deal with safely posting the messages to the DRM thread.
class DrmThreadProxy {
 public:
  DrmThreadProxy();
  ~DrmThreadProxy();

  void BindThreadIntoMessagingProxy(InterThreadMessagingProxy* messaging_proxy);

  void StartDrmThread(base::OnceClosure binding_drainer);

  std::unique_ptr<DrmWindowProxy> CreateDrmWindowProxy(
      gfx::AcceleratedWidget widget);

  scoped_refptr<GbmBuffer> CreateBuffer(gfx::AcceleratedWidget widget,
                                        const gfx::Size& size,
                                        gfx::BufferFormat format,
                                        gfx::BufferUsage usage);

  scoped_refptr<GbmBuffer> CreateBufferFromFds(
      gfx::AcceleratedWidget widget,
      const gfx::Size& size,
      gfx::BufferFormat format,
      std::vector<base::ScopedFD>&& fds,
      const std::vector<gfx::NativePixmapPlane>& planes);

  void GetScanoutFormats(gfx::AcceleratedWidget widget,
                         std::vector<gfx::BufferFormat>* scanout_formats);

  void AddBindingCursorDevice(ozone::mojom::DeviceCursorRequest request);
  void AddBindingDrmDevice(ozone::mojom::DrmDeviceRequest request);

 private:
  DrmThread drm_thread_;

  DISALLOW_COPY_AND_ASSIGN(DrmThreadProxy);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_THREAD_PROXY_H_
