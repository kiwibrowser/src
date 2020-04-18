// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/deferred_gpu_command_service.h"

#include "android_webview/browser/gl_view_renderer_manager.h"
#include "android_webview/browser/render_thread_manager.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/gpu_utils.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_info.h"
#include "gpu/config/gpu_util.h"
#include "ui/gl/gl_share_group.h"

namespace android_webview {

base::LazyInstance<base::ThreadLocalBoolean>::DestructorAtExit
    ScopedAllowGL::allow_gl;

// static
bool ScopedAllowGL::IsAllowed() {
  return allow_gl.Get().Get();
}

ScopedAllowGL::ScopedAllowGL() {
  DCHECK(!allow_gl.Get().Get());
  allow_gl.Get().Set(true);

  DeferredGpuCommandService* service = DeferredGpuCommandService::GetInstance();
  DCHECK(service);
  service->RunTasks();
}

ScopedAllowGL::~ScopedAllowGL() {
  allow_gl.Get().Set(false);

  DeferredGpuCommandService* service = DeferredGpuCommandService::GetInstance();
  DCHECK(service);
  service->RunTasks();
  if (service->IdleQueueSize()) {
    service->RequestProcessGL(true);
  }
}

// static
DeferredGpuCommandService*
DeferredGpuCommandService::CreateDeferredGpuCommandService() {
  gpu::GPUInfo gpu_info;
  gpu::GpuFeatureInfo gpu_feature_info;
  DCHECK(base::CommandLine::InitializedForCurrentProcess());
  gpu::GpuPreferences gpu_preferences =
      content::GetGpuPreferencesFromCommandLine();
  bool success = gpu::InitializeGLThreadSafe(
      base::CommandLine::ForCurrentProcess(),
      gpu_preferences.ignore_gpu_blacklist,
      gpu_preferences.disable_gpu_driver_bug_workarounds,
      gpu_preferences.log_gpu_control_list_decisions, &gpu_info,
      &gpu_feature_info);
  if (!success) {
    LOG(FATAL) << "gpu::InitializeGLThreadSafe() failed.";
  }
  return new DeferredGpuCommandService(gpu_preferences, gpu_info,
                                       gpu_feature_info);
}

// static
DeferredGpuCommandService* DeferredGpuCommandService::GetInstance() {
  static base::NoDestructor<scoped_refptr<DeferredGpuCommandService>> service(
      CreateDeferredGpuCommandService());
  return service->get();
}

DeferredGpuCommandService::DeferredGpuCommandService(
    const gpu::GpuPreferences& gpu_preferences,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info)
    : gpu::InProcessCommandBuffer::Service(gpu_preferences,
                                           nullptr,
                                           nullptr,
                                           gpu_feature_info),
      sync_point_manager_(new gpu::SyncPointManager()),
      gpu_info_(gpu_info) {}

DeferredGpuCommandService::~DeferredGpuCommandService() {
  base::AutoLock lock(tasks_lock_);
  DCHECK(tasks_.empty());
}

// This method can be called on any thread.
// static
void DeferredGpuCommandService::RequestProcessGL(bool for_idle) {
  RenderThreadManager* renderer_state =
      GLViewRendererManager::GetInstance()->GetMostRecentlyDrawn();
  if (!renderer_state) {
    LOG(ERROR) << "No hardware renderer. Deadlock likely";
    return;
  }
  renderer_state->ClientRequestInvokeGL(for_idle);
}

// Called from different threads!
void DeferredGpuCommandService::ScheduleTask(base::OnceClosure task) {
  {
    base::AutoLock lock(tasks_lock_);
    tasks_.push(std::move(task));
  }
  if (ScopedAllowGL::IsAllowed()) {
    RunTasks();
  } else {
    RequestProcessGL(false);
  }
}

size_t DeferredGpuCommandService::IdleQueueSize() {
  base::AutoLock lock(tasks_lock_);
  return idle_tasks_.size();
}

void DeferredGpuCommandService::ScheduleDelayedWork(
    base::OnceClosure callback) {
  {
    base::AutoLock lock(tasks_lock_);
    idle_tasks_.push(std::make_pair(base::Time::Now(), std::move(callback)));
  }
  RequestProcessGL(true);
}

void DeferredGpuCommandService::PerformIdleWork(bool is_idle) {
  TRACE_EVENT1("android_webview",
               "DeferredGpuCommandService::PerformIdleWork",
               "is_idle",
               is_idle);
  DCHECK(ScopedAllowGL::IsAllowed());
  static const base::TimeDelta kMaxIdleAge =
      base::TimeDelta::FromMilliseconds(16);

  const base::Time now = base::Time::Now();
  size_t queue_size = IdleQueueSize();
  while (queue_size--) {
    base::OnceClosure task;
    {
      base::AutoLock lock(tasks_lock_);
      if (!is_idle) {
        // Only run old tasks if we are not really idle right now.
        base::TimeDelta age(now - idle_tasks_.front().first);
        if (age < kMaxIdleAge)
          break;
      }
      task = std::move(idle_tasks_.front().second);
      idle_tasks_.pop();
    }
    std::move(task).Run();
  }
}

void DeferredGpuCommandService::PerformAllIdleWork() {
  TRACE_EVENT0("android_webview",
               "DeferredGpuCommandService::PerformAllIdleWork");
  while (IdleQueueSize()) {
    PerformIdleWork(true);
  }
}

bool DeferredGpuCommandService::ForceVirtualizedGLContexts() {
  return true;
}

gpu::SyncPointManager* DeferredGpuCommandService::sync_point_manager() {
  return sync_point_manager_.get();
}

void DeferredGpuCommandService::RunTasks() {
  TRACE_EVENT0("android_webview", "DeferredGpuCommandService::RunTasks");
  bool has_more_tasks;
  {
    base::AutoLock lock(tasks_lock_);
    has_more_tasks = tasks_.size() > 0;
  }

  while (has_more_tasks) {
    base::OnceClosure task;
    {
      base::AutoLock lock(tasks_lock_);
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    std::move(task).Run();
    {
      base::AutoLock lock(tasks_lock_);
      has_more_tasks = tasks_.size() > 0;
    }
  }
}

void DeferredGpuCommandService::AddRef() const {
  base::RefCountedThreadSafe<DeferredGpuCommandService>::AddRef();
}

void DeferredGpuCommandService::Release() const {
  base::RefCountedThreadSafe<DeferredGpuCommandService>::Release();
}

bool DeferredGpuCommandService::BlockThreadOnWaitSyncToken() const {
  return true;
}

bool DeferredGpuCommandService::CanSupportThreadedTextureMailbox() const {
  return gpu_info_.can_support_threaded_texture_mailbox;
}

}  // namespace android_webview
