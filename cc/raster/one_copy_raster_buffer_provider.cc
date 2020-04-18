// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/one_copy_raster_buffer_provider.h"

#include <stdint.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/histograms.h"
#include "cc/base/math_util.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/gpu/raster_context_provider.h"
#include "components/viz/common/gpu/texture_allocation.h"
#include "components/viz/common/resources/platform_color.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/trace_util.h"

namespace cc {
namespace {

// 4MiB is the size of 4 512x512 tiles, which has proven to be a good
// default batch size for copy operations.
const int kMaxBytesPerCopyOperation = 1024 * 1024 * 4;

}  // namespace

// Subclass for InUsePoolResource that holds ownership of a one-copy backing
// and does cleanup of the backing when destroyed.
class OneCopyRasterBufferProvider::OneCopyGpuBacking
    : public ResourcePool::GpuBacking {
 public:
  ~OneCopyGpuBacking() override {
    gpu::gles2::GLES2Interface* gl = compositor_context_provider->ContextGL();
    if (returned_sync_token.HasData())
      gl->WaitSyncTokenCHROMIUM(returned_sync_token.GetConstData());
    if (mailbox_sync_token.HasData())
      gl->WaitSyncTokenCHROMIUM(mailbox_sync_token.GetConstData());
    if (texture_id)
      gl->DeleteTextures(1, &texture_id);
  }

  base::trace_event::MemoryAllocatorDumpGuid MemoryDumpGuid(
      uint64_t tracing_process_id) override {
    if (!storage_allocated)
      return {};
    return gl::GetGLTextureClientGUIDForTracing(
        compositor_context_provider->ContextSupport()->ShareGroupTracingGUID(),
        texture_id);
  }
  base::UnguessableToken SharedMemoryGuid() override { return {}; }

  // The ContextProvider used to clean up the texture id.
  viz::ContextProvider* compositor_context_provider = nullptr;
  // The texture backing of the resource.
  GLuint texture_id = 0;
  // The allocation of storage for the |texture_id| is deferred, and this tracks
  // if it has been done.
  bool storage_allocated = false;
};

OneCopyRasterBufferProvider::RasterBufferImpl::RasterBufferImpl(
    OneCopyRasterBufferProvider* client,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    const ResourcePool::InUsePoolResource& in_use_resource,
    OneCopyGpuBacking* backing,
    uint64_t previous_content_id)
    : client_(client),
      backing_(backing),
      resource_size_(in_use_resource.size()),
      resource_format_(in_use_resource.format()),
      color_space_(in_use_resource.color_space()),
      previous_content_id_(previous_content_id),
      before_raster_sync_token_(backing->returned_sync_token),
      mailbox_(backing->mailbox),
      mailbox_texture_target_(backing->texture_target),
      mailbox_texture_is_overlay_candidate_(backing->overlay_candidate),
      mailbox_texture_storage_allocated_(backing->storage_allocated) {}

OneCopyRasterBufferProvider::RasterBufferImpl::~RasterBufferImpl() {
  // This SyncToken was created on the worker context after uploading the
  // texture content.
  backing_->mailbox_sync_token = after_raster_sync_token_;
  if (after_raster_sync_token_.HasData()) {
    // The returned SyncToken was waited on in Playback. We know Playback
    // happened if the |after_raster_sync_token_| was set.
    backing_->returned_sync_token = gpu::SyncToken();
  }
  backing_->storage_allocated = mailbox_texture_storage_allocated_;
}

void OneCopyRasterBufferProvider::RasterBufferImpl::Playback(
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    uint64_t new_content_id,
    const gfx::AxisTransform2d& transform,
    const RasterSource::PlaybackSettings& playback_settings) {
  TRACE_EVENT0("cc", "OneCopyRasterBuffer::Playback");
  // The |before_raster_sync_token_| passed in here was created on the
  // compositor thread, or given back with the texture for reuse. This call
  // returns another SyncToken generated on the worker thread to synchronize
  // with after the raster is complete.
  after_raster_sync_token_ = client_->PlaybackAndCopyOnWorkerThread(
      mailbox_, mailbox_texture_target_, mailbox_texture_is_overlay_candidate_,
      mailbox_texture_storage_allocated_, before_raster_sync_token_,
      raster_source, raster_full_rect, raster_dirty_rect, transform,
      resource_size_, resource_format_, color_space_, playback_settings,
      previous_content_id_, new_content_id);
  mailbox_texture_storage_allocated_ = true;
}

OneCopyRasterBufferProvider::OneCopyRasterBufferProvider(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    viz::ContextProvider* compositor_context_provider,
    viz::RasterContextProvider* worker_context_provider,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    int max_copy_texture_chromium_size,
    bool use_partial_raster,
    bool use_gpu_memory_buffer_resources,
    int max_staging_buffer_usage_in_bytes,
    viz::ResourceFormat tile_format)
    : compositor_context_provider_(compositor_context_provider),
      worker_context_provider_(worker_context_provider),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      max_bytes_per_copy_operation_(
          max_copy_texture_chromium_size
              ? std::min(kMaxBytesPerCopyOperation,
                         max_copy_texture_chromium_size)
              : kMaxBytesPerCopyOperation),
      use_partial_raster_(use_partial_raster),
      use_gpu_memory_buffer_resources_(use_gpu_memory_buffer_resources),
      bytes_scheduled_since_last_flush_(0),
      tile_format_(tile_format),
      staging_pool_(std::move(task_runner),
                    worker_context_provider,
                    use_partial_raster,
                    max_staging_buffer_usage_in_bytes) {
  DCHECK(compositor_context_provider);
  DCHECK(worker_context_provider);
}

OneCopyRasterBufferProvider::~OneCopyRasterBufferProvider() {}

std::unique_ptr<RasterBuffer>
OneCopyRasterBufferProvider::AcquireBufferForRaster(
    const ResourcePool::InUsePoolResource& resource,
    uint64_t resource_content_id,
    uint64_t previous_content_id) {
  if (!resource.gpu_backing()) {
    auto backing = std::make_unique<OneCopyGpuBacking>();
    backing->compositor_context_provider = compositor_context_provider_;

    gpu::gles2::GLES2Interface* gl = compositor_context_provider_->ContextGL();
    const auto& caps = compositor_context_provider_->ContextCapabilities();

    viz::TextureAllocation alloc = viz::TextureAllocation::MakeTextureId(
        gl, caps, resource.format(), use_gpu_memory_buffer_resources_,
        /*for_framebuffer_attachment=*/false);
    backing->texture_id = alloc.texture_id;
    backing->texture_target = alloc.texture_target;
    backing->overlay_candidate = alloc.overlay_candidate;
    backing->mailbox = gpu::Mailbox::Generate();
    gl->ProduceTextureDirectCHROMIUM(backing->texture_id,
                                     backing->mailbox.name);
    // Save a sync token in the backing so that we always wait on it even if
    // this task is cancelled between being scheduled and running.
    backing->returned_sync_token =
        LayerTreeResourceProvider::GenerateSyncTokenHelper(gl);

    resource.set_gpu_backing(std::move(backing));
  }
  OneCopyGpuBacking* backing =
      static_cast<OneCopyGpuBacking*>(resource.gpu_backing());
  // TODO(danakj): If resource_content_id != 0, we only need to copy/upload
  // the dirty rect.
  return std::make_unique<RasterBufferImpl>(
      this, gpu_memory_buffer_manager_, resource, backing, previous_content_id);
}

void OneCopyRasterBufferProvider::Flush() {
  // This flush on the compositor context flushes queued work on all contexts,
  // including the raster worker. Tile raster inserted a SyncToken which is
  // waited for in order to tell if a tile is ready for draw, but a flush
  // is needed to ensure the work is sent for those queries to get the right
  // answer.
  compositor_context_provider_->ContextSupport()->FlushPendingWork();
}

viz::ResourceFormat OneCopyRasterBufferProvider::GetResourceFormat() const {
  return tile_format_;
}

bool OneCopyRasterBufferProvider::IsResourceSwizzleRequired() const {
  return !viz::PlatformColor::SameComponentOrder(GetResourceFormat());
}

bool OneCopyRasterBufferProvider::IsResourcePremultiplied() const {
  // TODO(ericrk): Handle unpremultiply/dither in one-copy case as well.
  // https://crbug.com/789153
  return true;
}

bool OneCopyRasterBufferProvider::CanPartialRasterIntoProvidedResource() const {
  // While OneCopyRasterBufferProvider has an internal partial raster
  // implementation, it cannot directly partial raster into the externally
  // owned resource provided in AcquireBufferForRaster.
  return false;
}

bool OneCopyRasterBufferProvider::IsResourceReadyToDraw(
    const ResourcePool::InUsePoolResource& resource) const {
  const gpu::SyncToken& sync_token = resource.gpu_backing()->mailbox_sync_token;
  // This SyncToken() should have been set by calling OrderingBarrier() before
  // calling this.
  DCHECK(sync_token.HasData());

  // IsSyncTokenSignaled is thread-safe, no need for worker context lock.
  return worker_context_provider_->ContextSupport()->IsSyncTokenSignaled(
      sync_token);
}

uint64_t OneCopyRasterBufferProvider::SetReadyToDrawCallback(
    const std::vector<const ResourcePool::InUsePoolResource*>& resources,
    const base::Closure& callback,
    uint64_t pending_callback_id) const {
  gpu::SyncToken latest_sync_token;
  for (const auto* in_use : resources) {
    const gpu::SyncToken& sync_token =
        in_use->gpu_backing()->mailbox_sync_token;
    if (sync_token.release_count() > latest_sync_token.release_count())
      latest_sync_token = sync_token;
  }
  uint64_t callback_id = latest_sync_token.release_count();
  DCHECK_NE(callback_id, 0u);

  // If the callback is different from the one the caller is already waiting on,
  // pass the callback through to SignalSyncToken. Otherwise the request is
  // redundant.
  if (callback_id != pending_callback_id) {
    // Use the compositor context because we want this callback on the
    // compositor thread.
    compositor_context_provider_->ContextSupport()->SignalSyncToken(
        latest_sync_token, callback);
  }

  return callback_id;
}

void OneCopyRasterBufferProvider::Shutdown() {
  staging_pool_.Shutdown();
}

gpu::SyncToken OneCopyRasterBufferProvider::PlaybackAndCopyOnWorkerThread(
    const gpu::Mailbox& mailbox,
    GLenum mailbox_texture_target,
    bool mailbox_texture_is_overlay_candidate,
    bool mailbox_texture_storage_allocated,
    const gpu::SyncToken& sync_token,
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    const gfx::AxisTransform2d& transform,
    const gfx::Size& resource_size,
    viz::ResourceFormat resource_format,
    const gfx::ColorSpace& color_space,
    const RasterSource::PlaybackSettings& playback_settings,
    uint64_t previous_content_id,
    uint64_t new_content_id) {
  std::unique_ptr<StagingBuffer> staging_buffer =
      staging_pool_.AcquireStagingBuffer(resource_size, resource_format,
                                         previous_content_id);

  PlaybackToStagingBuffer(staging_buffer.get(), raster_source, raster_full_rect,
                          raster_dirty_rect, transform, resource_format,
                          color_space, playback_settings, previous_content_id,
                          new_content_id);

  gpu::SyncToken sync_token_after_upload = CopyOnWorkerThread(
      staging_buffer.get(), raster_source, raster_full_rect, resource_format,
      resource_size, mailbox, mailbox_texture_target,
      mailbox_texture_is_overlay_candidate, mailbox_texture_storage_allocated,
      sync_token, color_space);
  staging_pool_.ReleaseStagingBuffer(std::move(staging_buffer));
  return sync_token_after_upload;
}

void OneCopyRasterBufferProvider::PlaybackToStagingBuffer(
    StagingBuffer* staging_buffer,
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    const gfx::AxisTransform2d& transform,
    viz::ResourceFormat format,
    const gfx::ColorSpace& dst_color_space,
    const RasterSource::PlaybackSettings& playback_settings,
    uint64_t previous_content_id,
    uint64_t new_content_id) {
  // Allocate GpuMemoryBuffer if necessary. If using partial raster, we
  // must allocate a buffer with BufferUsage CPU_READ_WRITE_PERSISTENT.
  if (!staging_buffer->gpu_memory_buffer) {
    staging_buffer->gpu_memory_buffer =
        gpu_memory_buffer_manager_->CreateGpuMemoryBuffer(
            staging_buffer->size, BufferFormat(format), StagingBufferUsage(),
            gpu::kNullSurfaceHandle);
  }

  gfx::Rect playback_rect = raster_full_rect;
  if (use_partial_raster_ && previous_content_id) {
    // Reduce playback rect to dirty region if the content id of the staging
    // buffer matches the prevous content id.
    if (previous_content_id == staging_buffer->content_id)
      playback_rect.Intersect(raster_dirty_rect);
  }

  // Log a histogram of the percentage of pixels that were saved due to
  // partial raster.
  const char* client_name = GetClientNameForMetrics();
  float full_rect_size = raster_full_rect.size().GetArea();
  if (full_rect_size > 0 && client_name) {
    float fraction_partial_rastered =
        static_cast<float>(playback_rect.size().GetArea()) / full_rect_size;
    float fraction_saved = 1.0f - fraction_partial_rastered;
    UMA_HISTOGRAM_PERCENTAGE(
        base::StringPrintf("Renderer4.%s.PartialRasterPercentageSaved.OneCopy",
                           client_name),
        100.0f * fraction_saved);
  }

  if (staging_buffer->gpu_memory_buffer) {
    gfx::GpuMemoryBuffer* buffer = staging_buffer->gpu_memory_buffer.get();
    DCHECK_EQ(1u, gfx::NumberOfPlanesForBufferFormat(buffer->GetFormat()));
    bool rv = buffer->Map();
    DCHECK(rv);
    DCHECK(buffer->memory(0));
    // RasterBufferProvider::PlaybackToMemory only supports unsigned strides.
    DCHECK_GE(buffer->stride(0), 0);

    DCHECK(!playback_rect.IsEmpty())
        << "Why are we rastering a tile that's not dirty?";
    RasterBufferProvider::PlaybackToMemory(
        buffer->memory(0), format, staging_buffer->size, buffer->stride(0),
        raster_source, raster_full_rect, playback_rect, transform,
        dst_color_space, /*gpu_compositing=*/true, playback_settings);
    buffer->Unmap();
    staging_buffer->content_id = new_content_id;
  }
}

gpu::SyncToken OneCopyRasterBufferProvider::CopyOnWorkerThread(
    StagingBuffer* staging_buffer,
    const RasterSource* raster_source,
    const gfx::Rect& rect_to_copy,
    viz::ResourceFormat resource_format,
    const gfx::Size& resource_size,
    const gpu::Mailbox& mailbox,
    GLenum mailbox_texture_target,
    bool mailbox_texture_is_overlay_candidate,
    bool mailbox_texture_storage_allocated,
    const gpu::SyncToken& sync_token,
    const gfx::ColorSpace& color_space) {
  viz::RasterContextProvider::ScopedRasterContextLock scoped_context(
      worker_context_provider_);
  gpu::raster::RasterInterface* ri = scoped_context.RasterInterface();
  DCHECK(ri);

  // Wait on the SyncToken that was created on the compositor thread after
  // making the mailbox. This ensures that the mailbox we consume here is valid
  // by the time the consume command executes.
  ri->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  GLuint mailbox_texture_id = ri->CreateAndConsumeTexture(
      mailbox_texture_is_overlay_candidate, gfx::BufferUsage::SCANOUT,
      resource_format, mailbox.name);

  if (!mailbox_texture_storage_allocated) {
    viz::TextureAllocation alloc = {mailbox_texture_id, mailbox_texture_target,
                                    mailbox_texture_is_overlay_candidate};
    viz::TextureAllocation::AllocateStorage(
        ri, worker_context_provider_->ContextCapabilities(), resource_format,
        resource_size, alloc, color_space);
  }

  // Create and bind staging texture.
  if (!staging_buffer->texture_id) {
    staging_buffer->texture_id =
        ri->CreateTexture(true, StagingBufferUsage(), staging_buffer->format);
    ri->TexParameteri(staging_buffer->texture_id, GL_TEXTURE_MIN_FILTER,
                      GL_NEAREST);
    ri->TexParameteri(staging_buffer->texture_id, GL_TEXTURE_MAG_FILTER,
                      GL_NEAREST);
    ri->TexParameteri(staging_buffer->texture_id, GL_TEXTURE_WRAP_S,
                      GL_CLAMP_TO_EDGE);
    ri->TexParameteri(staging_buffer->texture_id, GL_TEXTURE_WRAP_T,
                      GL_CLAMP_TO_EDGE);
  }

  // Create and bind image.
  if (!staging_buffer->image_id) {
    if (staging_buffer->gpu_memory_buffer) {
      staging_buffer->image_id = ri->CreateImageCHROMIUM(
          staging_buffer->gpu_memory_buffer->AsClientBuffer(),
          staging_buffer->size.width(), staging_buffer->size.height(),
          GLInternalFormat(staging_buffer->format));
      ri->BindTexImage2DCHROMIUM(staging_buffer->texture_id,
                                 staging_buffer->image_id);
    }
  } else {
    ri->ReleaseTexImage2DCHROMIUM(staging_buffer->texture_id,
                                  staging_buffer->image_id);
    ri->BindTexImage2DCHROMIUM(staging_buffer->texture_id,
                               staging_buffer->image_id);
  }

  // Unbind staging texture.
  // TODO(vmiura): Need a way to ensure we don't hold onto bindings?
  // ri->BindTexture(image_target, 0);

  if (worker_context_provider_->ContextCapabilities().sync_query) {
    if (!staging_buffer->query_id)
      ri->GenQueriesEXT(1, &staging_buffer->query_id);

#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
    // TODO(reveman): This avoids a performance problem on ARM ChromeOS
    // devices. crbug.com/580166
    ri->BeginQueryEXT(GL_COMMANDS_ISSUED_CHROMIUM, staging_buffer->query_id);
#else
    ri->BeginQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM, staging_buffer->query_id);
#endif
  }

  // Since compressed texture's cannot be pre-allocated we might have an
  // unallocated resource in which case we need to perform a full size copy.
  if (IsResourceFormatCompressed(staging_buffer->format)) {
    ri->CompressedCopyTextureCHROMIUM(staging_buffer->texture_id,
                                      mailbox_texture_id);
  } else {
    int bytes_per_row = viz::ResourceSizes::UncheckedWidthInBytes<int>(
        rect_to_copy.width(), staging_buffer->format);
    int chunk_size_in_rows =
        std::max(1, max_bytes_per_copy_operation_ / bytes_per_row);
    // Align chunk size to 4. Required to support compressed texture formats.
    chunk_size_in_rows = MathUtil::UncheckedRoundUp(chunk_size_in_rows, 4);
    int y = 0;
    int height = rect_to_copy.height();
    while (y < height) {
      // Copy at most |chunk_size_in_rows|.
      int rows_to_copy = std::min(chunk_size_in_rows, height - y);
      DCHECK_GT(rows_to_copy, 0);

      ri->CopySubTexture(staging_buffer->texture_id, mailbox_texture_id, 0, y,
                         0, y, rect_to_copy.width(), rows_to_copy);
      y += rows_to_copy;

      // Increment |bytes_scheduled_since_last_flush_| by the amount of memory
      // used for this copy operation.
      bytes_scheduled_since_last_flush_ += rows_to_copy * bytes_per_row;

      if (bytes_scheduled_since_last_flush_ >= max_bytes_per_copy_operation_) {
        ri->ShallowFlushCHROMIUM();
        bytes_scheduled_since_last_flush_ = 0;
      }
    }
  }

  if (worker_context_provider_->ContextCapabilities().sync_query) {
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
    ri->EndQueryEXT(GL_COMMANDS_ISSUED_CHROMIUM);
#else
    ri->EndQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM);
#endif
  }

  ri->DeleteTextures(1, &mailbox_texture_id);

  // Generate sync token on the worker context that will be sent to and waited
  // for by the display compositor before using the content generated here.
  return LayerTreeResourceProvider::GenerateSyncTokenHelper(ri);
}

gfx::BufferUsage OneCopyRasterBufferProvider::StagingBufferUsage() const {
  return use_partial_raster_
             ? gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT
             : gfx::BufferUsage::GPU_READ_CPU_READ_WRITE;
}

}  // namespace cc
