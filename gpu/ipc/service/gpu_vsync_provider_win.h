// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_VSYNC_PROVIDER_WIN_H_
#define GPU_IPC_SERVICE_GPU_VSYNC_PROVIDER_WIN_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/service/gpu_ipc_service_export.h"
#include "gpu/ipc/service/image_transport_surface_delegate.h"
#include "ui/gfx/vsync_provider.h"

namespace gpu {

class GpuVSyncWorker;
class GpuVSyncMessageFilter;

class GPU_IPC_SERVICE_EXPORT GpuVSyncProviderWin : public gfx::VSyncProvider {
 public:
  GpuVSyncProviderWin(base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
                      SurfaceHandle surface_handle);
  ~GpuVSyncProviderWin() override;

  // This class ignores this method and updates VSync directly via a
  // worker thread IPC call.
  void GetVSyncParameters(const UpdateVSyncCallback& callback) override;
  bool GetVSyncParametersIfAvailable(base::TimeTicks* timebase,
                                     base::TimeDelta* interval) override;
  bool SupportGetVSyncParametersIfAvailable() const override;
  bool IsHWClock() const override;

 private:
  void OnVSync(base::TimeTicks timestamp, base::TimeDelta interval);

  // VSync worker that waits for VSync events on worker thread.
  scoped_refptr<GpuVSyncWorker> vsync_worker_;
  // Message filter to send/receive IPC messages.
  scoped_refptr<GpuVSyncMessageFilter> message_filter_;

  DISALLOW_COPY_AND_ASSIGN(GpuVSyncProviderWin);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_VSYNC_PROVIDER_WIN_H_
