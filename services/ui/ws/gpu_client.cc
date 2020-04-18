// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/gpu_client.h"

#include "components/viz/host/server_gpu_memory_buffer_manager.h"
#include "services/viz/privileged/interfaces/gl/gpu_service.mojom.h"

namespace ui {
namespace ws {

GpuClient::GpuClient(
    int client_id,
    gpu::GPUInfo* gpu_info,
    gpu::GpuFeatureInfo* gpu_feature_info,
    viz::ServerGpuMemoryBufferManager* gpu_memory_buffer_manager,
    viz::mojom::GpuService* gpu_service)
    : client_id_(client_id),
      gpu_info_(gpu_info),
      gpu_feature_info_(gpu_feature_info),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      gpu_service_(gpu_service),
      weak_factory_(this) {
  DCHECK(gpu_memory_buffer_manager_);
  DCHECK(gpu_service_);
}

GpuClient::~GpuClient() {
  gpu_memory_buffer_manager_->DestroyAllGpuMemoryBufferForClient(client_id_);
  if (!establish_callback_.is_null()) {
    std::move(establish_callback_)
        .Run(client_id_, mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
             gpu::GpuFeatureInfo());
  }
}

void GpuClient::OnGpuChannelEstablished(
    mojo::ScopedMessagePipeHandle channel_handle) {
  base::ResetAndReturn(&establish_callback_)
      .Run(client_id_, std::move(channel_handle), *gpu_info_,
           *gpu_feature_info_);
}

// mojom::Gpu overrides:
void GpuClient::EstablishGpuChannel(EstablishGpuChannelCallback callback) {
  // TODO(sad): crbug.com/617415 figure out how to generate a meaningful
  // tracing id.
  const uint64_t client_tracing_id = 0;
  constexpr bool is_gpu_host = false;
  if (!establish_callback_.is_null()) {
    std::move(establish_callback_)
        .Run(client_id_, mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
             gpu::GpuFeatureInfo());
  }
  establish_callback_ = std::move(callback);
  gpu_service_->EstablishGpuChannel(
      client_id_, client_tracing_id, is_gpu_host,
      base::Bind(&GpuClient::OnGpuChannelEstablished,
                 weak_factory_.GetWeakPtr()));
}

void GpuClient::CreateJpegDecodeAccelerator(
    media::mojom::JpegDecodeAcceleratorRequest jda_request) {
  gpu_service_->CreateJpegDecodeAccelerator(std::move(jda_request));
}

void GpuClient::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest request) {
  gpu_service_->CreateVideoEncodeAcceleratorProvider(std::move(request));
}

void GpuClient::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    mojom::Gpu::CreateGpuMemoryBufferCallback callback) {
  gpu_memory_buffer_manager_->AllocateGpuMemoryBuffer(
      id, client_id_, size, format, usage, gpu::kNullSurfaceHandle,
      std::move(callback));
}

void GpuClient::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                       const gpu::SyncToken& sync_token) {
  gpu_memory_buffer_manager_->DestroyGpuMemoryBuffer(id, client_id_,
                                                     sync_token);
}

}  // namespace ws
}  // namespace ui
