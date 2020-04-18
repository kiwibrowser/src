// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/gpu_in_process_thread_service.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_task_runner_handle.h"

namespace gpu {

GpuInProcessThreadService::GpuInProcessThreadService(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    gpu::SyncPointManager* sync_point_manager,
    gpu::MailboxManager* mailbox_manager,
    scoped_refptr<gl::GLShareGroup> share_group,
    const GpuFeatureInfo& gpu_feature_info,
    const GpuPreferences& gpu_preferences)
    : gpu::InProcessCommandBuffer::Service(gpu_preferences,
                                           mailbox_manager,
                                           share_group,
                                           gpu_feature_info),
      task_runner_(task_runner),
      sync_point_manager_(sync_point_manager) {}

void GpuInProcessThreadService::ScheduleTask(base::OnceClosure task) {
  task_runner_->PostTask(FROM_HERE, std::move(task));
}

void GpuInProcessThreadService::ScheduleDelayedWork(base::OnceClosure task) {
  task_runner_->PostDelayedTask(FROM_HERE, std::move(task),
                                base::TimeDelta::FromMilliseconds(2));
}

bool GpuInProcessThreadService::ForceVirtualizedGLContexts() {
  return false;
}

gpu::SyncPointManager* GpuInProcessThreadService::sync_point_manager() {
  return sync_point_manager_;
}

void GpuInProcessThreadService::AddRef() const {
  base::RefCountedThreadSafe<GpuInProcessThreadService>::AddRef();
}

void GpuInProcessThreadService::Release() const {
  base::RefCountedThreadSafe<GpuInProcessThreadService>::Release();
}

bool GpuInProcessThreadService::BlockThreadOnWaitSyncToken() const {
  return false;
}

GpuInProcessThreadService::~GpuInProcessThreadService() = default;

}  // namespace gpu
