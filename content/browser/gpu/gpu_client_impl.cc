// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/gpu_client_impl.h"

#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_shared_memory.h"

namespace content {

// static
std::unique_ptr<GpuClient, BrowserThread::DeleteOnIOThread> GpuClient::Create(
    ui::mojom::GpuRequest request,
    ConnectionErrorHandlerClosure connection_error_handler) {
  std::unique_ptr<GpuClientImpl, BrowserThread::DeleteOnIOThread> gpu_client(
      new GpuClientImpl(ChildProcessHostImpl::GenerateChildProcessUniqueId()));
  gpu_client->SetConnectionErrorHandler(std::move(connection_error_handler));
  gpu_client->Add(std::move(request));
  return gpu_client;
}

GpuClientImpl::GpuClientImpl(int render_process_id)
    : render_process_id_(render_process_id), weak_factory_(this) {
  bindings_.set_connection_error_handler(
      base::Bind(&GpuClientImpl::OnError, base::Unretained(this),
                 ErrorReason::kConnectionLost));
}

GpuClientImpl::~GpuClientImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bindings_.CloseAllBindings();
  OnError(ErrorReason::kInDestructor);
}

void GpuClientImpl::Add(ui::mojom::GpuRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void GpuClientImpl::OnError(ErrorReason reason) {
  ClearCallback();
  if (bindings_.empty()) {
    BrowserGpuMemoryBufferManager* gpu_memory_buffer_manager =
        BrowserGpuMemoryBufferManager::current();
    if (gpu_memory_buffer_manager)
      gpu_memory_buffer_manager->ProcessRemoved(render_process_id_);
  }
  if (reason == ErrorReason::kConnectionLost && connection_error_handler_)
    std::move(connection_error_handler_).Run(this);
}

void GpuClientImpl::PreEstablishGpuChannel() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&GpuClientImpl::EstablishGpuChannel,
                     base::Unretained(this), EstablishGpuChannelCallback()));
}

void GpuClientImpl::SetConnectionErrorHandler(
    ConnectionErrorHandlerClosure connection_error_handler) {
  connection_error_handler_ = std::move(connection_error_handler);
}

void GpuClientImpl::OnEstablishGpuChannel(
    mojo::ScopedMessagePipeHandle channel_handle,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info,
    GpuProcessHost::EstablishChannelStatus status) {
  DCHECK_EQ(channel_handle.is_valid(),
            status == GpuProcessHost::EstablishChannelStatus::SUCCESS);
  gpu_channel_requested_ = false;
  EstablishGpuChannelCallback callback = std::move(callback_);
  DCHECK(!callback_);

  if (status == GpuProcessHost::EstablishChannelStatus::GPU_HOST_INVALID) {
    // GPU process may have crashed or been killed. Try again.
    EstablishGpuChannel(std::move(callback));
    return;
  }
  if (callback) {
    // A request is waiting.
    std::move(callback).Run(render_process_id_, std::move(channel_handle),
                            gpu_info, gpu_feature_info);
    return;
  }
  if (status == GpuProcessHost::EstablishChannelStatus::SUCCESS) {
    // This is the case we pre-establish a channel before a request arrives.
    // Cache the channel for a future request.
    channel_handle_ = std::move(channel_handle);
    gpu_info_ = gpu_info;
    gpu_feature_info_ = gpu_feature_info;
  }
}

void GpuClientImpl::OnCreateGpuMemoryBuffer(
    CreateGpuMemoryBufferCallback callback,
    const gfx::GpuMemoryBufferHandle& handle) {
  std::move(callback).Run(handle);
}

void GpuClientImpl::ClearCallback() {
  if (!callback_)
    return;
  EstablishGpuChannelCallback callback = std::move(callback_);
  std::move(callback).Run(render_process_id_, mojo::ScopedMessagePipeHandle(),
                          gpu::GPUInfo(), gpu::GpuFeatureInfo());
  DCHECK(!callback_);
}

void GpuClientImpl::EstablishGpuChannel(EstablishGpuChannelCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // At most one channel should be requested. So clear previous request first.
  ClearCallback();
  if (channel_handle_.is_valid()) {
    // If a channel has been pre-established and cached,
    //   1) if callback is valid, return it right away.
    //   2) if callback is empty, it's PreEstablishGpyChannel() being called
    //      more than once, no need to do anything.
    if (callback) {
      std::move(callback).Run(render_process_id_, std::move(channel_handle_),
                              gpu_info_, gpu_feature_info_);
      DCHECK(!channel_handle_.is_valid());
    }
    return;
  }
  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host) {
    if (callback) {
      std::move(callback).Run(render_process_id_,
                              mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
                              gpu::GpuFeatureInfo());
    }
    return;
  }
  callback_ = std::move(callback);
  if (gpu_channel_requested_)
    return;
  gpu_channel_requested_ = true;
  bool preempts = false;
  bool allow_view_command_buffers = false;
  bool allow_real_time_streams = false;
  host->EstablishGpuChannel(
      render_process_id_,
      ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
          render_process_id_),
      preempts, allow_view_command_buffers, allow_real_time_streams,
      base::Bind(&GpuClientImpl::OnEstablishGpuChannel,
                 weak_factory_.GetWeakPtr()));
}

void GpuClientImpl::CreateJpegDecodeAccelerator(
    media::mojom::JpegDecodeAcceleratorRequest jda_request) {
  GpuProcessHost* host = GpuProcessHost::Get();
  if (host)
    host->gpu_service()->CreateJpegDecodeAccelerator(std::move(jda_request));
}

void GpuClientImpl::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest vea_provider_request) {
  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host)
    return;
  host->gpu_service()->CreateVideoEncodeAcceleratorProvider(
      std::move(vea_provider_request));
}

void GpuClientImpl::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    ui::mojom::Gpu::CreateGpuMemoryBufferCallback callback) {
  DCHECK(BrowserGpuMemoryBufferManager::current());

  base::CheckedNumeric<int> bytes = size.width();
  bytes *= size.height();
  if (!bytes.IsValid()) {
    OnCreateGpuMemoryBuffer(std::move(callback), gfx::GpuMemoryBufferHandle());
    return;
  }

  BrowserGpuMemoryBufferManager::current()
      ->AllocateGpuMemoryBufferForChildProcess(
          id, size, format, usage, render_process_id_,
          base::BindOnce(&GpuClientImpl::OnCreateGpuMemoryBuffer,
                         weak_factory_.GetWeakPtr(), std::move(callback)));
}

void GpuClientImpl::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                           const gpu::SyncToken& sync_token) {
  DCHECK(BrowserGpuMemoryBufferManager::current());

  BrowserGpuMemoryBufferManager::current()->ChildProcessDeletedGpuMemoryBuffer(
      id, render_process_id_, sync_token);
}

}  // namespace content
