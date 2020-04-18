// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_DEFERRED_GPU_COMMAND_SERVICE_H_
#define ANDROID_WEBVIEW_BROWSER_DEFERRED_GPU_COMMAND_SERVICE_H_

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/containers/queue.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_local.h"
#include "base/time/time.h"
#include "gpu/config/gpu_info.h"
#include "gpu/ipc/in_process_command_buffer.h"

namespace gpu {
struct GpuFeatureInfo;
struct GpuPreferences;
class SyncPointManager;
}

namespace android_webview {

class ScopedAllowGL {
 public:
  ScopedAllowGL();
  ~ScopedAllowGL();

  static bool IsAllowed();

 private:
  static base::LazyInstance<base::ThreadLocalBoolean>::DestructorAtExit
      allow_gl;

  DISALLOW_COPY_AND_ASSIGN(ScopedAllowGL);
};

class DeferredGpuCommandService
    : public gpu::InProcessCommandBuffer::Service,
      public base::RefCountedThreadSafe<DeferredGpuCommandService> {
 public:
  static DeferredGpuCommandService* GetInstance();

  void ScheduleTask(base::OnceClosure task) override;
  void ScheduleDelayedWork(base::OnceClosure task) override;
  bool ForceVirtualizedGLContexts() override;
  gpu::SyncPointManager* sync_point_manager() override;

  void RunTasks();
  // If |is_idle| is false, this will only run older idle tasks.
  void PerformIdleWork(bool is_idle);
  // Flush the idle queue until it is empty. This is different from
  // PerformIdleWork(is_idle = true), which does not run any newly scheduled
  // idle tasks during the idle run.
  void PerformAllIdleWork();

  void AddRef() const override;
  void Release() const override;
  bool BlockThreadOnWaitSyncToken() const override;

  const gpu::GPUInfo& gpu_info() const { return gpu_info_; }

  bool CanSupportThreadedTextureMailbox() const;

 protected:
  ~DeferredGpuCommandService() override;
  friend class base::RefCountedThreadSafe<DeferredGpuCommandService>;

 private:
  friend class ScopedAllowGL;
  static void RequestProcessGL(bool for_idle);

  DeferredGpuCommandService(const gpu::GpuPreferences& gpu_preferences,
                            const gpu::GPUInfo& gpu_info,
                            const gpu::GpuFeatureInfo& gpu_feature_info);

  static DeferredGpuCommandService* CreateDeferredGpuCommandService();

  size_t IdleQueueSize();

  base::Lock tasks_lock_;
  base::queue<base::OnceClosure> tasks_;
  base::queue<std::pair<base::Time, base::OnceClosure>> idle_tasks_;

  std::unique_ptr<gpu::SyncPointManager> sync_point_manager_;
  gpu::GPUInfo gpu_info_;

  DISALLOW_COPY_AND_ASSIGN(DeferredGpuCommandService);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_DEFERRED_GPU_COMMAND_SERVICE_H_
