// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/gpu/gpu_video_accelerator_factories_impl.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "components/viz/common/gpu/context_provider.h"
#include "content/child/child_thread_impl.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_names.mojom.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "media/gpu/gpu_video_accelerator_util.h"
#include "media/gpu/ipc/client/gpu_video_decode_accelerator_host.h"
#include "media/gpu/ipc/common/media_messages.h"
#include "media/mojo/clients/mojo_video_encode_accelerator.h"
#include "media/video/video_decode_accelerator.h"
#include "media/video/video_encode_accelerator.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

namespace content {

namespace {

// This enum values match ContextProviderPhase in histograms.xml
enum ContextProviderPhase {
  CONTEXT_PROVIDER_ACQUIRED = 0,
  CONTEXT_PROVIDER_RELEASED = 1,
  CONTEXT_PROVIDER_RELEASED_MAX_VALUE = CONTEXT_PROVIDER_RELEASED,
};

void RecordContextProviderPhaseUmaEnum(const ContextProviderPhase phase) {
  UMA_HISTOGRAM_ENUMERATION("Media.GPU.HasEverLostContext", phase,
                            CONTEXT_PROVIDER_RELEASED_MAX_VALUE + 1);
}

}  // namespace

// static
std::unique_ptr<GpuVideoAcceleratorFactoriesImpl>
GpuVideoAcceleratorFactoriesImpl::Create(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<ui::ContextProviderCommandBuffer>& context_provider,
    bool enable_video_gpu_memory_buffers,
    bool enable_media_stream_gpu_memory_buffers,
    bool enable_video_accelerator,
    media::mojom::VideoEncodeAcceleratorProviderPtrInfo unbound_vea_provider) {
  RecordContextProviderPhaseUmaEnum(
      ContextProviderPhase::CONTEXT_PROVIDER_ACQUIRED);
  return base::WrapUnique(new GpuVideoAcceleratorFactoriesImpl(
      std::move(gpu_channel_host), main_thread_task_runner, task_runner,
      context_provider, enable_video_gpu_memory_buffers,
      enable_media_stream_gpu_memory_buffers, enable_video_accelerator,
      std::move(unbound_vea_provider)));
}

GpuVideoAcceleratorFactoriesImpl::GpuVideoAcceleratorFactoriesImpl(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<ui::ContextProviderCommandBuffer>& context_provider,
    bool enable_video_gpu_memory_buffers,
    bool enable_media_stream_gpu_memory_buffers,
    bool enable_video_accelerator,
    media::mojom::VideoEncodeAcceleratorProviderPtrInfo unbound_vea_provider)
    : main_thread_task_runner_(main_thread_task_runner),
      task_runner_(task_runner),
      gpu_channel_host_(std::move(gpu_channel_host)),
      context_provider_(context_provider),
      context_provider_lost_(false),
      enable_video_gpu_memory_buffers_(enable_video_gpu_memory_buffers),
      enable_media_stream_gpu_memory_buffers_(
          enable_media_stream_gpu_memory_buffers),
      video_accelerator_enabled_(enable_video_accelerator),
      gpu_memory_buffer_manager_(
          RenderThreadImpl::current()->GetGpuMemoryBufferManager()),
      thread_safe_sender_(ChildThreadImpl::current()->thread_safe_sender()) {
  DCHECK(main_thread_task_runner_);
  DCHECK(gpu_channel_host_);

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuVideoAcceleratorFactoriesImpl::BindContextToTaskRunner,
                     base::Unretained(this)));

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuVideoAcceleratorFactoriesImpl::
                         BindVideoEncodeAcceleratorProviderOnTaskRunner,
                     base::Unretained(this), std::move(unbound_vea_provider)));
}

GpuVideoAcceleratorFactoriesImpl::~GpuVideoAcceleratorFactoriesImpl() {}

void GpuVideoAcceleratorFactoriesImpl::BindContextToTaskRunner() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(context_provider_);
  if (context_provider_->BindToCurrentThread() !=
      gpu::ContextResult::kSuccess) {
    ReleaseContextProvider();
  }
}

bool GpuVideoAcceleratorFactoriesImpl::CheckContextLost() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (context_provider_) {
    if (context_provider_->ContextGL()->GetGraphicsResetStatusKHR() !=
        GL_NO_ERROR) {
      ReleaseContextProvider();
    }
  }
  return !context_provider_;
}

bool GpuVideoAcceleratorFactoriesImpl::IsGpuVideoAcceleratorEnabled() {
  return video_accelerator_enabled_;
}

base::UnguessableToken GpuVideoAcceleratorFactoriesImpl::GetChannelToken() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return base::UnguessableToken();

  if (channel_token_.is_empty()) {
    context_provider_->GetCommandBufferProxy()->channel()->Send(
        new GpuCommandBufferMsg_GetChannelToken(&channel_token_));
  }

  return channel_token_;
}

int32_t GpuVideoAcceleratorFactoriesImpl::GetCommandBufferRouteId() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return 0;
  return context_provider_->GetCommandBufferProxy()->route_id();
}

std::unique_ptr<media::VideoDecodeAccelerator>
GpuVideoAcceleratorFactoriesImpl::CreateVideoDecodeAccelerator() {
  DCHECK(video_accelerator_enabled_);
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return nullptr;

  return std::unique_ptr<media::VideoDecodeAccelerator>(
      new media::GpuVideoDecodeAcceleratorHost(
          context_provider_->GetCommandBufferProxy()));
}

std::unique_ptr<media::VideoEncodeAccelerator>
GpuVideoAcceleratorFactoriesImpl::CreateVideoEncodeAccelerator() {
  DCHECK(video_accelerator_enabled_);
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(vea_provider_.is_bound());
  if (CheckContextLost())
    return nullptr;

  media::mojom::VideoEncodeAcceleratorPtr vea;
  vea_provider_->CreateVideoEncodeAccelerator(mojo::MakeRequest(&vea));

  if (!vea)
    return nullptr;

  return std::unique_ptr<media::VideoEncodeAccelerator>(
      new media::MojoVideoEncodeAccelerator(
          std::move(vea), context_provider_->GetCommandBufferProxy()
                              ->channel()
                              ->gpu_info()
                              .video_encode_accelerator_supported_profiles));
}

bool GpuVideoAcceleratorFactoriesImpl::CreateTextures(
    int32_t count,
    const gfx::Size& size,
    std::vector<uint32_t>* texture_ids,
    std::vector<gpu::Mailbox>* texture_mailboxes,
    uint32_t texture_target) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(texture_target);

  if (CheckContextLost())
    return false;
  gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
  texture_ids->resize(count);
  texture_mailboxes->resize(count);
  gles2->GenTextures(count, &texture_ids->at(0));
  for (int i = 0; i < count; ++i) {
    gles2->ActiveTexture(GL_TEXTURE0);
    uint32_t texture_id = texture_ids->at(i);
    gles2->BindTexture(texture_target, texture_id);
    gles2->TexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gles2->TexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gles2->TexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gles2->TexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (texture_target == GL_TEXTURE_2D) {
      gles2->TexImage2D(texture_target, 0, GL_RGBA, size.width(), size.height(),
                        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    gles2->GenMailboxCHROMIUM(texture_mailboxes->at(i).name);
    gles2->ProduceTextureDirectCHROMIUM(texture_id,
                                        texture_mailboxes->at(i).name);
  }

  // We need ShallowFlushCHROMIUM() here to order the command buffer commands
  // with respect to IPC to the GPU process, to guarantee that the decoder in
  // the GPU process can use these textures as soon as it receives IPC
  // notification of them.
  gles2->ShallowFlushCHROMIUM();
  DCHECK_EQ(gles2->GetError(), static_cast<GLenum>(GL_NO_ERROR));
  return true;
}

void GpuVideoAcceleratorFactoriesImpl::DeleteTexture(uint32_t texture_id) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return;

  gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
  gles2->DeleteTextures(1, &texture_id);
  DCHECK_EQ(gles2->GetError(), static_cast<GLenum>(GL_NO_ERROR));
}

gpu::SyncToken GpuVideoAcceleratorFactoriesImpl::CreateSyncToken() {
  gpu::SyncToken sync_token;
  context_provider_->ContextGL()->GenSyncTokenCHROMIUM(sync_token.GetData());
  return sync_token;
}

void GpuVideoAcceleratorFactoriesImpl::WaitSyncToken(
    const gpu::SyncToken& sync_token) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return;

  gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
  gles2->WaitSyncTokenCHROMIUM(sync_token.GetConstData());

  // Callers expect the WaitSyncToken to affect the next IPCs. Make sure to
  // flush the command buffers to ensure that.
  gles2->ShallowFlushCHROMIUM();
}

void GpuVideoAcceleratorFactoriesImpl::ShallowFlushCHROMIUM() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return;

  context_provider_->ContextGL()->ShallowFlushCHROMIUM();
}

std::unique_ptr<gfx::GpuMemoryBuffer>
GpuVideoAcceleratorFactoriesImpl::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return gpu_memory_buffer_manager_->CreateGpuMemoryBuffer(
      size, format, usage, gpu::kNullSurfaceHandle);
}
bool GpuVideoAcceleratorFactoriesImpl::ShouldUseGpuMemoryBuffersForVideoFrames(
    bool for_media_stream) const {
  return for_media_stream ? enable_media_stream_gpu_memory_buffers_
                          : enable_video_gpu_memory_buffers_;
}

unsigned GpuVideoAcceleratorFactoriesImpl::ImageTextureTarget(
    gfx::BufferFormat format) {
  DCHECK(context_provider_);
  return gpu::GetBufferTextureTarget(gfx::BufferUsage::GPU_READ_CPU_READ_WRITE,
                                     format,
                                     context_provider_->ContextCapabilities());
}

media::GpuVideoAcceleratorFactories::OutputFormat
GpuVideoAcceleratorFactoriesImpl::VideoFrameOutputFormat(size_t bit_depth) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (CheckContextLost())
    return media::GpuVideoAcceleratorFactories::OutputFormat::UNDEFINED;
  auto capabilities = context_provider_->ContextCapabilities();
  if (bit_depth > 8) {
    // If high bit depth rendering is enabled, bail here, otherwise try and use
    // XR30 storage, and if not and we support RG textures, use those, albeit at
    // a reduced bit depth of 8 bits per component.
    // TODO(mcasas): continue working on this, avoiding dropping information as
    // long as the hardware may support it https://crbug.com/798485.
    if (rendering_color_space_.IsHDR())
      return media::GpuVideoAcceleratorFactories::OutputFormat::UNDEFINED;

#if defined(OS_MACOSX) || defined(OS_LINUX)
    // TODO(mcasas): enable other platforms https://crbug.com/776093
    // https://crbug.com/803451.
    // TODO(mcasas): remove the |bit_depth| check when libyuv supports more than
    // just x010ToAR30 conversions, https://crbug.com/libyuv/751.
    if (bit_depth == 10) {
      if (capabilities.image_xr30)
        return media::GpuVideoAcceleratorFactories::OutputFormat::XR30;
      else if (capabilities.image_xb30)
        return media::GpuVideoAcceleratorFactories::OutputFormat::XB30;
    }
#endif
    if (capabilities.texture_rg)
      return media::GpuVideoAcceleratorFactories::OutputFormat::I420;
    return media::GpuVideoAcceleratorFactories::OutputFormat::UNDEFINED;
  }
  if (capabilities.image_ycbcr_420v &&
      !capabilities.image_ycbcr_420v_disabled_for_video_frames) {
    return media::GpuVideoAcceleratorFactories::OutputFormat::NV12_SINGLE_GMB;
  }
  if (capabilities.image_ycbcr_422)
    return media::GpuVideoAcceleratorFactories::OutputFormat::UYVY;
  if (capabilities.texture_rg)
    return media::GpuVideoAcceleratorFactories::OutputFormat::NV12_DUAL_GMB;
  return media::GpuVideoAcceleratorFactories::OutputFormat::UNDEFINED;
}

gpu::gles2::GLES2Interface* GpuVideoAcceleratorFactoriesImpl::ContextGL() {
  return CheckContextLost() ? nullptr : context_provider_->ContextGL();
}

std::unique_ptr<base::SharedMemory>
GpuVideoAcceleratorFactoriesImpl::CreateSharedMemory(size_t size) {
  std::unique_ptr<base::SharedMemory> mem(
      ChildThreadImpl::AllocateSharedMemory(size));
  if (mem && !mem->Map(size))
    return nullptr;
  return mem;
}

scoped_refptr<base::SingleThreadTaskRunner>
GpuVideoAcceleratorFactoriesImpl::GetTaskRunner() {
  return task_runner_;
}

media::VideoDecodeAccelerator::Capabilities
GpuVideoAcceleratorFactoriesImpl::GetVideoDecodeAcceleratorCapabilities() {
  return media::GpuVideoAcceleratorUtil::ConvertGpuToMediaDecodeCapabilities(
      gpu_channel_host_->gpu_info().video_decode_accelerator_capabilities);
}

media::VideoEncodeAccelerator::SupportedProfiles
GpuVideoAcceleratorFactoriesImpl::GetVideoEncodeAcceleratorSupportedProfiles() {
  return media::GpuVideoAcceleratorUtil::ConvertGpuToMediaEncodeProfiles(
      gpu_channel_host_->gpu_info()
          .video_encode_accelerator_supported_profiles);
}

scoped_refptr<ui::ContextProviderCommandBuffer>
GpuVideoAcceleratorFactoriesImpl::GetMediaContextProvider() {
  return CheckContextLost() ? nullptr : context_provider_;
}

void GpuVideoAcceleratorFactoriesImpl::SetRenderingColorSpace(
    const gfx::ColorSpace& color_space) {
  rendering_color_space_ = color_space;
}

bool GpuVideoAcceleratorFactoriesImpl::CheckContextProviderLost() {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  return context_provider_lost_;
}

void GpuVideoAcceleratorFactoriesImpl::
    BindVideoEncodeAcceleratorProviderOnTaskRunner(
        media::mojom::VideoEncodeAcceleratorProviderPtrInfo
            unbound_vea_provider) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!vea_provider_.is_bound());
  vea_provider_.Bind(std::move(unbound_vea_provider), task_runner_);
}

void GpuVideoAcceleratorFactoriesImpl::ReleaseContextProvider() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuVideoAcceleratorFactoriesImpl::SetContextProviderLost,
                     base::Unretained(this)));

  context_provider_ = nullptr;
  RecordContextProviderPhaseUmaEnum(
      ContextProviderPhase::CONTEXT_PROVIDER_RELEASED);
}

void GpuVideoAcceleratorFactoriesImpl::SetContextProviderLost() {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  context_provider_lost_ = true;
}

}  // namespace content
