// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/server_gpu_memory_buffer_manager.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_shared_memory.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "services/viz/privileged/interfaces/gl/gpu_service.mojom.h"
#include "ui/gfx/buffer_format_util.h"

namespace viz {

namespace {

void OnGpuMemoryBufferDestroyed(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const gpu::GpuMemoryBufferImpl::DestructionCallback& callback,
    const gpu::SyncToken& sync_token) {
  task_runner->PostTask(FROM_HERE, base::Bind(callback, sync_token));
}

}  // namespace

ServerGpuMemoryBufferManager::BufferInfo::BufferInfo() = default;
ServerGpuMemoryBufferManager::BufferInfo::~BufferInfo() = default;

ServerGpuMemoryBufferManager::ServerGpuMemoryBufferManager(
    mojom::GpuService* gpu_service,
    int client_id,
    std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support)
    : gpu_service_(gpu_service),
      client_id_(client_id),
      gpu_memory_buffer_support_(std::move(gpu_memory_buffer_support)),
      native_configurations_(gpu::GetNativeGpuMemoryBufferConfigurations(
          gpu_memory_buffer_support_.get())),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

ServerGpuMemoryBufferManager::~ServerGpuMemoryBufferManager() {}

void ServerGpuMemoryBufferManager::AllocateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle,
    base::OnceCallback<void(const gfx::GpuMemoryBufferHandle&)> callback) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  if (gpu_memory_buffer_support_->GetNativeGpuMemoryBufferType() !=
      gfx::EMPTY_BUFFER) {
    const bool is_native = native_configurations_.find(std::make_pair(
                               format, usage)) != native_configurations_.end();
    if (is_native) {
      pending_buffers_.insert(client_id);
      gpu_service_->CreateGpuMemoryBuffer(
          id, size, format, usage, client_id, surface_handle,
          base::BindOnce(
              &ServerGpuMemoryBufferManager::OnGpuMemoryBufferAllocated,
              weak_ptr_, client_id,
              gfx::BufferSizeForBufferFormat(size, format),
              std::move(callback)));
      return;
    }
  }

  gfx::GpuMemoryBufferHandle buffer_handle;
  // The requests are coming in from untrusted clients. So verify that it is
  // possible to allocate shared memory buffer first.
  if (gpu::GpuMemoryBufferImplSharedMemory::IsUsageSupported(usage) &&
      gpu::GpuMemoryBufferImplSharedMemory::IsSizeValidForFormat(size,
                                                                 format)) {
    buffer_handle = gpu::GpuMemoryBufferImplSharedMemory::CreateGpuMemoryBuffer(
        id, size, format, usage);
    BufferInfo buffer_info;
    DCHECK_EQ(gfx::SHARED_MEMORY_BUFFER, buffer_handle.type);
    buffer_info.type = gfx::SHARED_MEMORY_BUFFER;
    buffer_info.buffer_size_in_bytes =
        gfx::BufferSizeForBufferFormat(size, format);
    buffer_info.shared_memory_guid = buffer_handle.handle.GetGUID();
    allocated_buffers_[client_id].insert(
        std::make_pair(buffer_handle.id, buffer_info));
  }

  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(std::move(callback), buffer_handle));
}

std::unique_ptr<gfx::GpuMemoryBuffer>
ServerGpuMemoryBufferManager::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  gfx::GpuMemoryBufferId id(next_gpu_memory_id_++);
  gfx::GpuMemoryBufferHandle handle;
  base::WaitableEvent wait_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  DCHECK(!task_runner_->RunsTasksInCurrentSequence());
  auto reply_callback = base::BindOnce(
      [](gfx::GpuMemoryBufferHandle* handle, base::WaitableEvent* wait_event,
         const gfx::GpuMemoryBufferHandle& allocated_buffer_handle) {
        *handle = allocated_buffer_handle;
        wait_event->Signal();
      },
      &handle, &wait_event);
  // We block with a WaitableEvent until the callback is run. So using
  // base::Unretained() is safe here.
  auto allocate_callback =
      base::BindOnce(&ServerGpuMemoryBufferManager::AllocateGpuMemoryBuffer,
                     base::Unretained(this), id, client_id_, size, format,
                     usage, surface_handle, std::move(reply_callback));
  task_runner_->PostTask(FROM_HERE, std::move(allocate_callback));
  base::ThreadRestrictions::ScopedAllowWait allow_wait;
  wait_event.Wait();
  if (handle.is_null())
    return nullptr;
  // The destruction callback can be called on any thread. So use an
  // intermediate callback here as the destruction callback, which bounces off
  // onto the |task_runner_| thread to do the real work.
  return gpu_memory_buffer_support_->CreateGpuMemoryBufferImplFromHandle(
      handle, size, format, usage,
      base::Bind(
          &OnGpuMemoryBufferDestroyed, task_runner_,
          base::Bind(&ServerGpuMemoryBufferManager::DestroyGpuMemoryBuffer,
                     weak_ptr_, id, client_id_)));
}

void ServerGpuMemoryBufferManager::SetDestructionSyncToken(
    gfx::GpuMemoryBuffer* buffer,
    const gpu::SyncToken& sync_token) {
  static_cast<gpu::GpuMemoryBufferImpl*>(buffer)->set_destruction_sync_token(
      sync_token);
}

bool ServerGpuMemoryBufferManager::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  for (const auto& pair : allocated_buffers_) {
    int client_id = pair.first;
    for (const auto& buffer_pair : pair.second) {
      gfx::GpuMemoryBufferId buffer_id = buffer_pair.first;
      const BufferInfo& buffer_info = buffer_pair.second;
      base::trace_event::MemoryAllocatorDump* dump =
          pmd->CreateAllocatorDump(base::StringPrintf(
              "gpumemorybuffer/client_%d/buffer_%d", client_id, buffer_id.id));
      if (!dump)
        return false;
      dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                      base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                      buffer_info.buffer_size_in_bytes);

      // Create the cross-process ownership edge. If the client creates a
      // corresponding dump for the same buffer, this will avoid to
      // double-count them in tracing. If, instead, no other process will emit a
      // dump with the same guid, the segment will be accounted to the browser.
      uint64_t client_tracing_process_id = ClientIdToTracingId(client_id);

      if (buffer_info.type == gfx::SHARED_MEMORY_BUFFER) {
        pmd->CreateSharedMemoryOwnershipEdge(
            dump->guid(), buffer_info.shared_memory_guid, 0 /* importance */);
      } else {
        auto shared_buffer_guid = gfx::GetGenericSharedGpuMemoryGUIDForTracing(
            client_tracing_process_id, buffer_id);
        pmd->CreateSharedGlobalAllocatorDump(shared_buffer_guid);
        pmd->AddOwnershipEdge(dump->guid(), shared_buffer_guid);
      }
    }
  }
  return true;
}

void ServerGpuMemoryBufferManager::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id,
    const gpu::SyncToken& sync_token) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  auto iter = allocated_buffers_[client_id].find(id);
  if (iter == allocated_buffers_[client_id].end())
    return;
  DCHECK_NE(gfx::EMPTY_BUFFER, iter->second.type);
  if (iter->second.type != gfx::SHARED_MEMORY_BUFFER)
    gpu_service_->DestroyGpuMemoryBuffer(id, client_id, sync_token);
  allocated_buffers_[client_id].erase(id);
}

void ServerGpuMemoryBufferManager::DestroyAllGpuMemoryBufferForClient(
    int client_id) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  for (auto pair : allocated_buffers_[client_id]) {
    DCHECK_NE(gfx::EMPTY_BUFFER, pair.second.type);
    if (pair.second.type != gfx::SHARED_MEMORY_BUFFER) {
      gpu_service_->DestroyGpuMemoryBuffer(pair.first, client_id,
                                           gpu::SyncToken());
    }
  }
  allocated_buffers_.erase(client_id);
  pending_buffers_.erase(client_id);
}

uint64_t ServerGpuMemoryBufferManager::ClientIdToTracingId(
    int client_id) const {
  if (client_id == client_id_) {
    return base::trace_event::MemoryDumpManager::GetInstance()
        ->GetTracingProcessId();
  }
  // TODO(sad|ssid): Find a better way once crbug.com/661257 is resolved.
  // The hash value is incremented so that the tracing id is never equal to
  // MemoryDumpManager::kInvalidTracingProcessId.
  return static_cast<uint64_t>(base::Hash(&client_id, sizeof(client_id))) + 1;
}

void ServerGpuMemoryBufferManager::OnGpuMemoryBufferAllocated(
    int client_id,
    size_t buffer_size_in_bytes,
    base::OnceCallback<void(const gfx::GpuMemoryBufferHandle&)> callback,
    const gfx::GpuMemoryBufferHandle& handle) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  if (pending_buffers_.find(client_id) == pending_buffers_.end()) {
    // The client has been destroyed since the allocation request was made.
    if (!handle.is_null()) {
      gpu_service_->DestroyGpuMemoryBuffer(handle.id, client_id,
                                           gpu::SyncToken());
    }
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }
  if (!handle.is_null()) {
    BufferInfo buffer_info;
    buffer_info.type = handle.type;
    buffer_info.buffer_size_in_bytes = buffer_size_in_bytes;
    allocated_buffers_[client_id].insert(
        std::make_pair(handle.id, buffer_info));
  }
  std::move(callback).Run(handle);
}

}  // namespace viz
