// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_shared_memory.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_switches.h"

namespace content {
namespace {

void GpuMemoryBufferDeleted(
    scoped_refptr<base::SingleThreadTaskRunner> destruction_task_runner,
    const gpu::GpuMemoryBufferImpl::DestructionCallback& destruction_callback,
    const gpu::SyncToken& sync_token) {
  destruction_task_runner->PostTask(
      FROM_HERE, base::BindOnce(destruction_callback, sync_token));
}

BrowserGpuMemoryBufferManager* g_gpu_memory_buffer_manager = nullptr;

}  // namespace

struct BrowserGpuMemoryBufferManager::CreateGpuMemoryBufferRequest {
  CreateGpuMemoryBufferRequest(const gfx::Size& size,
                               gfx::BufferFormat format,
                               gfx::BufferUsage usage,
                               int client_id,
                               gpu::SurfaceHandle surface_handle)
      : event(base::WaitableEvent::ResetPolicy::MANUAL,
              base::WaitableEvent::InitialState::NOT_SIGNALED),
        size(size),
        format(format),
        usage(usage),
        client_id(client_id),
        surface_handle(surface_handle) {}
  ~CreateGpuMemoryBufferRequest() {}
  base::WaitableEvent event;
  gfx::Size size;
  gfx::BufferFormat format;
  gfx::BufferUsage usage;
  int client_id;
  gpu::SurfaceHandle surface_handle;
  std::unique_ptr<gfx::GpuMemoryBuffer> result;
};

BrowserGpuMemoryBufferManager::BrowserGpuMemoryBufferManager(
    int gpu_client_id,
    uint64_t gpu_client_tracing_id)
    : gpu_memory_buffer_support_(new gpu::GpuMemoryBufferSupport()),
      native_configurations_(gpu::GetNativeGpuMemoryBufferConfigurations(
          gpu_memory_buffer_support_.get())),
      gpu_client_id_(gpu_client_id),
      gpu_client_tracing_id_(gpu_client_tracing_id) {
  DCHECK(!g_gpu_memory_buffer_manager);
  g_gpu_memory_buffer_manager = this;

  // Enable the dump provider with IO thread affinity. Note that
  // unregistration happens on the IO thread (See
  // BrowserProcessSubThread::IOThreadPreCleanUp).
  DCHECK(BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "BrowserGpuMemoryBufferManager",
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
}

BrowserGpuMemoryBufferManager::~BrowserGpuMemoryBufferManager() {
  g_gpu_memory_buffer_manager = nullptr;
}

// static
BrowserGpuMemoryBufferManager* BrowserGpuMemoryBufferManager::current() {
  return g_gpu_memory_buffer_manager;
}

std::unique_ptr<gfx::GpuMemoryBuffer>
BrowserGpuMemoryBufferManager::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  return AllocateGpuMemoryBufferForSurface(size, format, usage, surface_handle);
}

void BrowserGpuMemoryBufferManager::AllocateGpuMemoryBufferForChildProcess(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int child_client_id,
    AllocationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Use service side allocation for native configurations.
  if (IsNativeGpuMemoryBufferConfiguration(format, usage)) {
    CreateGpuMemoryBufferOnIO(id, size, format, usage, gpu::kNullSurfaceHandle,
                              child_client_id, std::move(callback));
    return;
  }

  // Early out if we cannot fallback to shared memory buffer.
  if (!gpu::GpuMemoryBufferImplSharedMemory::IsUsageSupported(usage) ||
      !gpu::GpuMemoryBufferImplSharedMemory::IsSizeValidForFormat(size,
                                                                  format)) {
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }

  BufferMap& buffers = clients_[child_client_id];

  // Allocate shared memory buffer as fallback.
  auto insert_result = buffers.insert(std::make_pair(
      id, BufferInfo(size, gfx::SHARED_MEMORY_BUFFER, format, usage, 0)));
  if (!insert_result.second) {
    DLOG(ERROR) << "Child process attempted to allocate a GpuMemoryBuffer with "
                   "an existing ID.";
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }

  auto handle = gpu::GpuMemoryBufferImplSharedMemory::CreateGpuMemoryBuffer(
      id, size, format, usage);
  buffers.find(id)->second.shared_memory_guid = handle.handle.GetGUID();
  std::move(callback).Run(handle);
}

void BrowserGpuMemoryBufferManager::SetDestructionSyncToken(
    gfx::GpuMemoryBuffer* buffer,
    const gpu::SyncToken& sync_token) {
  static_cast<gpu::GpuMemoryBufferImpl*>(buffer)->set_destruction_sync_token(
      sync_token);
}

bool BrowserGpuMemoryBufferManager::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (const auto& client : clients_) {
    int client_id = client.first;

    for (const auto& buffer : client.second) {
      if (buffer.second.type == gfx::EMPTY_BUFFER)
        continue;

      gfx::GpuMemoryBufferId buffer_id = buffer.first;
      base::trace_event::MemoryAllocatorDump* dump =
          pmd->CreateAllocatorDump(base::StringPrintf(
              "gpumemorybuffer/client_%d/buffer_%d", client_id, buffer_id.id));
      if (!dump)
        return false;

      size_t buffer_size_in_bytes = gfx::BufferSizeForBufferFormat(
          buffer.second.size, buffer.second.format);
      dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                      base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                      buffer_size_in_bytes);

      // Create the cross-process ownership edge. If the client creates a
      // corresponding dump for the same buffer, this will avoid to
      // double-count them in tracing. If, instead, no other process will emit a
      // dump with the same guid, the segment will be accounted to the browser.
      uint64_t client_tracing_process_id =
          ClientIdToTracingProcessId(client_id);

      if (buffer.second.type == gfx::SHARED_MEMORY_BUFFER) {
        pmd->CreateSharedMemoryOwnershipEdge(
            dump->guid(), buffer.second.shared_memory_guid, 0 /* importance */);
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

void BrowserGpuMemoryBufferManager::ChildProcessDeletedGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int child_client_id,
    const gpu::SyncToken& sync_token) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DestroyGpuMemoryBufferOnIO(id, child_client_id, sync_token);
}

void BrowserGpuMemoryBufferManager::ProcessRemoved(
    int client_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ClientMap::iterator client_it = clients_.find(client_id);
  if (client_it == clients_.end())
    return;

  for (const auto& buffer : client_it->second) {
    // This might happen if buffer is currenlty in the process of being
    // allocated. The buffer will in that case be cleaned up when allocation
    // completes.
    if (buffer.second.type == gfx::EMPTY_BUFFER)
      continue;

    GpuProcessHost* host = GpuProcessHost::FromID(buffer.second.gpu_host_id);
    if (host)
      host->DestroyGpuMemoryBuffer(buffer.first, client_id, gpu::SyncToken());
  }

  clients_.erase(client_it);
}

bool BrowserGpuMemoryBufferManager::IsNativeGpuMemoryBufferConfiguration(
    gfx::BufferFormat format,
    gfx::BufferUsage usage) const {
  return native_configurations_.find(std::make_pair(format, usage)) !=
         native_configurations_.end();
}

std::unique_ptr<gfx::GpuMemoryBuffer>
BrowserGpuMemoryBufferManager::AllocateGpuMemoryBufferForSurface(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));

  CreateGpuMemoryBufferRequest request(size, format, usage, gpu_client_id_,
                                       surface_handle);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &BrowserGpuMemoryBufferManager::HandleCreateGpuMemoryBufferOnIO,
          base::Unretained(this),  // Safe as we wait for result below.
          base::Unretained(&request)));

  // We're blocking the UI thread, which is generally undesirable.
  TRACE_EVENT0(
      "browser",
      "BrowserGpuMemoryBufferManager::AllocateGpuMemoryBufferForSurface");
  base::ThreadRestrictions::ScopedAllowWait allow_wait;
  request.event.Wait();
  return std::move(request.result);
}

void BrowserGpuMemoryBufferManager::HandleCreateGpuMemoryBufferOnIO(
    CreateGpuMemoryBufferRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  gfx::GpuMemoryBufferId new_id(next_gpu_memory_id_++);
  // Use service side allocation for native configurations.
  if (IsNativeGpuMemoryBufferConfiguration(request->format, request->usage)) {
    // Note: Unretained is safe as this is only used for synchronous allocation
    // from a non-IO thread.
    CreateGpuMemoryBufferOnIO(
        new_id, request->size, request->format, request->usage,
        request->surface_handle, request->client_id,
        base::BindOnce(
            &BrowserGpuMemoryBufferManager::HandleGpuMemoryBufferCreatedOnIO,
            base::Unretained(this), base::Unretained(request)));
    return;
  }

  DCHECK(gpu::GpuMemoryBufferImplSharedMemory::IsUsageSupported(request->usage))
      << static_cast<int>(request->usage);

  BufferMap& buffers = clients_[request->client_id];

  // Allocate shared memory buffer as fallback.
  auto insert_result = buffers.insert(std::make_pair(
      new_id, BufferInfo(request->size, gfx::SHARED_MEMORY_BUFFER,
                         request->format, request->usage, 0)));
  DCHECK(insert_result.second);

  // Note: Unretained is safe as IO thread is stopped before manager is
  // destroyed.
  request->result = gpu::GpuMemoryBufferImplSharedMemory::Create(
      new_id, request->size, request->format, request->usage,
      base::Bind(
          &GpuMemoryBufferDeleted,
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
          base::Bind(&BrowserGpuMemoryBufferManager::DestroyGpuMemoryBufferOnIO,
                     base::Unretained(this), new_id, request->client_id)));
  if (request->result) {
    buffers.find(new_id)->second.shared_memory_guid =
        request->result->GetHandle().handle.GetGUID();
  }
  request->event.Signal();
}

void BrowserGpuMemoryBufferManager::HandleGpuMemoryBufferCreatedOnIO(
    CreateGpuMemoryBufferRequest* request,
    const gfx::GpuMemoryBufferHandle& handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Early out if factory failed to create the buffer.
  if (handle.is_null()) {
    request->event.Signal();
    return;
  }

  // Note: Unretained is safe as IO thread is stopped before manager is
  // destroyed.
  request->result =
      gpu_memory_buffer_support_->CreateGpuMemoryBufferImplFromHandle(
          handle, request->size, request->format, request->usage,
          base::Bind(
              &GpuMemoryBufferDeleted,
              BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
              base::Bind(
                  &BrowserGpuMemoryBufferManager::DestroyGpuMemoryBufferOnIO,
                  base::Unretained(this), handle.id, request->client_id)));
  request->event.Signal();
}

void BrowserGpuMemoryBufferManager::CreateGpuMemoryBufferOnIO(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle,
    int client_id,
    CreateCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BufferMap& buffers = clients_[client_id];

  // Note: Handling of cases where the client is removed before the allocation
  // completes is less subtle if we set the buffer type to EMPTY_BUFFER here
  // and verify that this has not changed when creation completes.
  auto insert_result = buffers.insert(std::make_pair(
      id, BufferInfo(size, gfx::EMPTY_BUFFER, format, usage, 0)));
  if (!insert_result.second) {
    DLOG(ERROR) << "Child process attempted to create a GpuMemoryBuffer with "
                   "an existing ID.";
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }

  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host) {
    DLOG(ERROR) << "Cannot allocate GpuMemoryBuffer with no GpuProcessHost.";
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }
  // Note: Unretained is safe as IO thread is stopped before manager is
  // destroyed.
  host->CreateGpuMemoryBuffer(
      id, size, format, usage, client_id, surface_handle,
      base::BindOnce(&BrowserGpuMemoryBufferManager::GpuMemoryBufferCreatedOnIO,
                     base::Unretained(this), id, surface_handle, client_id,
                     host->host_id(), std::move(callback)));
}

void BrowserGpuMemoryBufferManager::GpuMemoryBufferCreatedOnIO(
    gfx::GpuMemoryBufferId id,
    gpu::SurfaceHandle surface_handle,
    int client_id,
    int gpu_host_id,
    CreateCallback callback,
    const gfx::GpuMemoryBufferHandle& handle,
    GpuProcessHost::BufferCreationStatus status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ClientMap::iterator client_it = clients_.find(client_id);

  // This can happen if client is removed while the buffer is being allocated.
  if (client_it == clients_.end()) {
    if (!handle.is_null()) {
      GpuProcessHost* host = GpuProcessHost::FromID(gpu_host_id);
      if (host)
        host->DestroyGpuMemoryBuffer(handle.id, client_id, gpu::SyncToken());
    }
    std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    return;
  }

  BufferMap& buffers = client_it->second;

  BufferMap::iterator buffer_it = buffers.find(id);
  DCHECK(buffer_it != buffers.end());
  DCHECK_EQ(buffer_it->second.type, gfx::EMPTY_BUFFER);

  // If the handle isn't valid, that means that the GPU process crashed or is
  // misbehaving.
  bool valid_handle = !handle.is_null() && handle.id == id;
  if (!valid_handle) {
    // If we failed after re-using the GPU process, it may have died in the
    // mean time. Retry to have a chance to create a fresh GPU process.
    if (handle.is_null() &&
        status == GpuProcessHost::BufferCreationStatus::GPU_HOST_INVALID) {
      DVLOG(1) << "Failed to create buffer through existing GPU process. "
                  "Trying to restart GPU process.";
      gfx::Size size = buffer_it->second.size;
      gfx::BufferFormat format = buffer_it->second.format;
      gfx::BufferUsage usage = buffer_it->second.usage;
      // Remove the buffer entry and call CreateGpuMemoryBufferOnIO again.
      buffers.erase(buffer_it);
      CreateGpuMemoryBufferOnIO(id, size, format, usage, surface_handle,
                                client_id, std::move(callback));
    } else {
      // Remove the buffer entry and run the allocation callback with an empty
      // handle to indicate failure.
      buffers.erase(buffer_it);
      std::move(callback).Run(gfx::GpuMemoryBufferHandle());
    }
    return;
  }

  // Store the type and host id of this buffer so it can be cleaned up if the
  // client is removed.
  buffer_it->second.type = handle.type;
  buffer_it->second.gpu_host_id = gpu_host_id;
  buffer_it->second.shared_memory_guid = handle.handle.GetGUID();

  std::move(callback).Run(handle);
}

void BrowserGpuMemoryBufferManager::DestroyGpuMemoryBufferOnIO(
    gfx::GpuMemoryBufferId id,
    int client_id,
    const gpu::SyncToken& sync_token) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(clients_.find(client_id) != clients_.end());

  BufferMap& buffers = clients_[client_id];

  BufferMap::iterator buffer_it = buffers.find(id);
  if (buffer_it == buffers.end()) {
    LOG(ERROR) << "Invalid GpuMemoryBuffer ID for client.";
    return;
  }

  // This can happen if a client managed to call this while a buffer is in the
  // process of being allocated.
  if (buffer_it->second.type == gfx::EMPTY_BUFFER) {
    LOG(ERROR) << "Invalid GpuMemoryBuffer type.";
    return;
  }

  GpuProcessHost* host = GpuProcessHost::FromID(buffer_it->second.gpu_host_id);
  if (host)
    host->DestroyGpuMemoryBuffer(id, client_id, sync_token);

  buffers.erase(buffer_it);
}

uint64_t BrowserGpuMemoryBufferManager::ClientIdToTracingProcessId(
    int client_id) const {
  if (client_id == gpu_client_id_) {
    // The gpu_client uses a fixed tracing ID.
    return gpu_client_tracing_id_;
  }

  // In normal cases, |client_id| is a child process id, so we can perform
  // the standard conversion.
  return ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
      client_id);
}

BrowserGpuMemoryBufferManager::BufferInfo::BufferInfo(
    const gfx::Size& size,
    gfx::GpuMemoryBufferType type,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int gpu_host_id)
    : size(size),
      type(type),
      format(format),
      usage(usage),
      gpu_host_id(gpu_host_id) {}

BrowserGpuMemoryBufferManager::BufferInfo::BufferInfo(const BufferInfo& other) =
    default;

BrowserGpuMemoryBufferManager::BufferInfo::~BufferInfo() {}

}  // namespace content
