// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/client_gpu_memory_buffer_manager.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/gfx/buffer_format_util.h"

using DestructionCallback = base::Callback<void(const gpu::SyncToken& sync)>;

namespace gpu {
class GpuMemoryBufferSupport;
}

namespace ui {

namespace {

void NotifyDestructionOnCorrectThread(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const DestructionCallback& callback,
    const gpu::SyncToken& sync_token) {
  task_runner->PostTask(FROM_HERE, base::Bind(callback, sync_token));
}

}  // namespace

ClientGpuMemoryBufferManager::ClientGpuMemoryBufferManager(mojom::GpuPtr gpu)
    : thread_("GpuMemoryThread"),
      gpu_memory_buffer_support_(
          std::make_unique<gpu::GpuMemoryBufferSupport>()),
      weak_ptr_factory_(this) {
  CHECK(thread_.Start());
  // The thread is owned by this object. Which means the task will not run if
  // the object has been destroyed. So Unretained() is safe.
  thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ClientGpuMemoryBufferManager::InitThread,
                 base::Unretained(this), base::Passed(gpu.PassInterface())));
}

ClientGpuMemoryBufferManager::~ClientGpuMemoryBufferManager() {
  thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&ClientGpuMemoryBufferManager::TearDownThread,
                            base::Unretained(this)));
  thread_.Stop();
}

void ClientGpuMemoryBufferManager::InitThread(mojom::GpuPtrInfo gpu_info) {
  gpu_.Bind(std::move(gpu_info));
  gpu_.set_connection_error_handler(
      base::Bind(&ClientGpuMemoryBufferManager::DisconnectGpuOnThread,
                 base::Unretained(this)));
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
}

void ClientGpuMemoryBufferManager::TearDownThread() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  DisconnectGpuOnThread();
}

void ClientGpuMemoryBufferManager::DisconnectGpuOnThread() {
  if (!gpu_.is_bound())
    return;
  gpu_.reset();
  for (auto* waiter : pending_allocation_waiters_)
    waiter->Signal();
  pending_allocation_waiters_.clear();
}

void ClientGpuMemoryBufferManager::AllocateGpuMemoryBufferOnThread(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gfx::GpuMemoryBufferHandle* handle,
    base::WaitableEvent* wait) {
  DCHECK(thread_.task_runner()->BelongsToCurrentThread());

  if (!gpu_) {
    // The Gpu interface may have been disconnected, in which case we can't
    // fulfill the request.
    wait->Signal();
    return;
  }

  // |handle| and |wait| are both on the stack, and will be alive until |wait|
  // is signaled. So it is safe for OnGpuMemoryBufferAllocated() to operate on
  // these.
  pending_allocation_waiters_.insert(wait);
  gpu_->CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId(++counter_), size, format, usage,
      base::Bind(
          &ClientGpuMemoryBufferManager::OnGpuMemoryBufferAllocatedOnThread,
          base::Unretained(this), handle, wait));
}

void ClientGpuMemoryBufferManager::OnGpuMemoryBufferAllocatedOnThread(
    gfx::GpuMemoryBufferHandle* ret_handle,
    base::WaitableEvent* wait,
    const gfx::GpuMemoryBufferHandle& handle) {
  auto it = pending_allocation_waiters_.find(wait);
  DCHECK(it != pending_allocation_waiters_.end());
  pending_allocation_waiters_.erase(it);

  *ret_handle = handle;
  wait->Signal();
}

void ClientGpuMemoryBufferManager::DeletedGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gpu::SyncToken& sync_token) {
  if (!thread_.task_runner()->BelongsToCurrentThread()) {
    thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&ClientGpuMemoryBufferManager::DeletedGpuMemoryBuffer,
                   base::Unretained(this), id, sync_token));
    return;
  }

  if (gpu_)
    gpu_->DestroyGpuMemoryBuffer(id, sync_token);
}

std::unique_ptr<gfx::GpuMemoryBuffer>
ClientGpuMemoryBufferManager::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  // Note: this can be called from multiple threads at the same time. Some of
  // those threads may not have a TaskRunner set.
  DCHECK_EQ(gpu::kNullSurfaceHandle, surface_handle);
  CHECK(!thread_.task_runner()->BelongsToCurrentThread());
  gfx::GpuMemoryBufferHandle gmb_handle;
  base::WaitableEvent wait(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ClientGpuMemoryBufferManager::AllocateGpuMemoryBufferOnThread,
                 base::Unretained(this), size, format, usage, &gmb_handle,
                 &wait));
  wait.Wait();
  if (gmb_handle.is_null())
    return nullptr;

  DestructionCallback callback =
      base::Bind(&ClientGpuMemoryBufferManager::DeletedGpuMemoryBuffer,
                 weak_ptr_, gmb_handle.id);
  std::unique_ptr<gpu::GpuMemoryBufferImpl> buffer(
      gpu_memory_buffer_support_->CreateGpuMemoryBufferImplFromHandle(
          gmb_handle, size, format, usage,
          base::Bind(&NotifyDestructionOnCorrectThread, thread_.task_runner(),
                     callback)));
  if (!buffer) {
    DeletedGpuMemoryBuffer(gmb_handle.id, gpu::SyncToken());
    return nullptr;
  }
  return std::move(buffer);
}

void ClientGpuMemoryBufferManager::SetDestructionSyncToken(
    gfx::GpuMemoryBuffer* buffer,
    const gpu::SyncToken& sync_token) {
  static_cast<gpu::GpuMemoryBufferImpl*>(buffer)->set_destruction_sync_token(
      sync_token);
}

}  // namespace ui
