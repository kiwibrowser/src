// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_RESOURCE_PROVIDER_H_
#define CC_RESOURCES_RESOURCE_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/small_map.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/memory_dump_provider.h"
#include "cc/cc_export.h"
#include "cc/resources/return_callback.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "components/viz/common/resources/release_callback.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/resources/resource_settings.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {
class GpuMemoryBufferManager;
namespace gles {
class GLES2Interface;
}
}

namespace viz {
class SharedBitmap;
class SharedBitmapManager;
}  // namespace viz

namespace cc {
class TextureIdAllocator;

// This class provides abstractions for allocating and transferring resources
// between modules/threads/processes. It abstracts away GL textures vs
// GpuMemoryBuffers vs software bitmaps behind a single ResourceId so that
// code in common can hold onto ResourceIds, as long as the code using them
// knows the correct type.
//
// The resource's underlying type is accessed through Read and Write locks that
// help to safeguard correct usage with DCHECKs. All resources held in
// ResourceProvider are immutable - they can not change format or size once
// they are created, only their contents.
//
// This class is not thread-safe and can only be called from the thread it was
// created on (in practice, the impl thread).
class CC_EXPORT ResourceProvider
    : public base::trace_event::MemoryDumpProvider {
 protected:
  struct Resource;

 public:
  using ResourceIdArray = std::vector<viz::ResourceId>;
  using ResourceIdMap = std::unordered_map<viz::ResourceId, viz::ResourceId>;
  enum TextureHint {
    TEXTURE_HINT_DEFAULT = 0x0,
    TEXTURE_HINT_MIPMAP = 0x1,
    TEXTURE_HINT_FRAMEBUFFER = 0x2,
    TEXTURE_HINT_MIPMAP_FRAMEBUFFER =
        TEXTURE_HINT_MIPMAP | TEXTURE_HINT_FRAMEBUFFER
  };
  enum ResourceType {
    RESOURCE_TYPE_GPU_MEMORY_BUFFER,
    RESOURCE_TYPE_GL_TEXTURE,
    RESOURCE_TYPE_BITMAP,
  };

  ResourceProvider(viz::ContextProvider* compositor_context_provider,
                   viz::SharedBitmapManager* shared_bitmap_manager,
                   gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                   bool delegated_sync_points_required,
                   const viz::ResourceSettings& resource_settings);
  ~ResourceProvider() override;

  static bool IsGpuResourceType(ResourceProvider::ResourceType type) {
    return type != ResourceProvider::RESOURCE_TYPE_BITMAP;
  }

  void Initialize();

  void DidLoseVulkanContextProvider() { lost_context_provider_ = true; }

  int max_texture_size() const { return settings_.max_texture_size; }
  viz::ResourceFormat best_texture_format() const {
    return settings_.best_texture_format;
  }
  viz::ResourceFormat best_render_buffer_format() const {
    return settings_.best_render_buffer_format;
  }
  viz::ResourceFormat YuvResourceFormat(int bits) const;
  bool use_sync_query() const { return settings_.use_sync_query; }
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager() {
    return gpu_memory_buffer_manager_;
  }
  size_t num_resources() const { return resources_.size(); }

  bool IsTextureFormatSupported(viz::ResourceFormat format) const;

  // Returns true if the provided |format| can be used as a render buffer.
  // Note that render buffer support implies texture support.
  bool IsRenderBufferFormatSupported(viz::ResourceFormat format) const;

  // Checks whether a resource is in use by a consumer.
  bool InUseByConsumer(viz::ResourceId id);

  bool IsLost(viz::ResourceId id);

  void LoseResourceForTesting(viz::ResourceId id);
  void EnableReadLockFencesForTesting(viz::ResourceId id);

  // Producer interface.

  ResourceType default_resource_type() const {
    return settings_.default_resource_type;
  }
  ResourceType GetResourceType(viz::ResourceId id);
  GLenum GetResourceTextureTarget(viz::ResourceId id);
  TextureHint GetTextureHint(viz::ResourceId id);

  // Creates a resource of the default resource type.
  viz::ResourceId CreateResource(const gfx::Size& size,
                                 TextureHint hint,
                                 viz::ResourceFormat format,
                                 const gfx::ColorSpace& color_space);

  // Creates a resource for a particular texture target (the distinction between
  // texture targets has no effect in software mode).
  viz::ResourceId CreateGpuMemoryBufferResource(
      const gfx::Size& size,
      TextureHint hint,
      viz::ResourceFormat format,
      gfx::BufferUsage usage,
      const gfx::ColorSpace& color_space);

  void DeleteResource(viz::ResourceId id);
  // In the case of GPU resources, we may need to flush the GL context to ensure
  // that texture deletions are seen in a timely fashion. This function should
  // be called after texture deletions that may happen during an idle state.
  void FlushPendingDeletions() const;

  // Update pixels from image, copying source_rect (in image) to dest_offset (in
  // the resource).
  void CopyToResource(viz::ResourceId id,
                      const uint8_t* image,
                      const gfx::Size& image_size);


  // The following lock classes are part of the ResourceProvider API and are
  // needed to read and write the resource contents. The user must ensure
  // that they only use GL locks on GL resources, etc, and this is enforced
  // by assertions.
  class CC_EXPORT ScopedWriteLockGL {
   public:
    ScopedWriteLockGL(ResourceProvider* resource_provider,
                      viz::ResourceId resource_id);
    ~ScopedWriteLockGL();

    GLenum target() const { return target_; }
    viz::ResourceFormat format() const { return format_; }
    const gfx::Size& size() const { return size_; }
    const gfx::ColorSpace& color_space_for_raster() const {
      return color_space_;
    }

    void set_sync_token(const gpu::SyncToken& sync_token) {
      sync_token_ = sync_token;
      has_sync_token_ = true;
    }

    GrPixelConfig PixelConfig() const;

    void set_synchronized() { synchronized_ = true; }

    void set_generate_mipmap() { generate_mipmap_ = true; }

    // Returns texture id on compositor context, allocating if necessary.
    GLuint GetTexture();

    // Creates mailbox that can be consumed on another context.
    void CreateMailbox();

    // Creates a texture id, allocating if necessary, on the given context. The
    // texture id must be deleted by the caller.
    GLuint ConsumeTexture(gpu::gles2::GLES2Interface* gl);

   private:
    void LazyAllocate(gpu::gles2::GLES2Interface* gl, GLuint texture_id);

    void AllocateGpuMemoryBuffer(gpu::gles2::GLES2Interface* gl,
                                 GLuint texture_id);

    void AllocateTexture(gpu::gles2::GLES2Interface* gl, GLuint texture_id);

    ResourceProvider* const resource_provider_;
    const viz::ResourceId resource_id_;

    // The following are copied from the resource.
    ResourceProvider::ResourceType type_;
    gfx::Size size_;
    viz::ResourceFormat format_;
    gfx::BufferUsage usage_;
    gfx::ColorSpace color_space_;
    GLuint texture_id_;
    GLenum target_;
    ResourceProvider::TextureHint hint_;
    std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
    GLuint image_id_;
    gpu::Mailbox mailbox_;
    bool allocated_;

    // Set by the user.
    gpu::SyncToken sync_token_;
    bool has_sync_token_ = false;
    bool synchronized_ = false;
    bool generate_mipmap_ = false;

    DISALLOW_COPY_AND_ASSIGN(ScopedWriteLockGL);
  };

  // TODO(sunnyps): Move to //components/viz/common/gl_helper.h ?
  class CC_EXPORT ScopedSkSurface {
   public:
    ScopedSkSurface(GrContext* gr_context,
                    GLuint texture_id,
                    GLenum texture_target,
                    const gfx::Size& size,
                    viz::ResourceFormat format,
                    bool use_distance_field_text,
                    bool can_use_lcd_text,
                    int msaa_sample_count);
    ~ScopedSkSurface();

    SkSurface* surface() const { return surface_.get(); }

   private:
    sk_sp<SkSurface> surface_;

    DISALLOW_COPY_AND_ASSIGN(ScopedSkSurface);
  };

  class CC_EXPORT ScopedWriteLockSoftware {
   public:
    ScopedWriteLockSoftware(ResourceProvider* resource_provider,
                            viz::ResourceId resource_id);
    ~ScopedWriteLockSoftware();

    SkBitmap& sk_bitmap() { return sk_bitmap_; }
    bool valid() const { return !!sk_bitmap_.getPixels(); }
    const gfx::ColorSpace& color_space_for_raster() const {
      return color_space_;
    }

   private:
    ResourceProvider* const resource_provider_;
    const viz::ResourceId resource_id_;
    gfx::ColorSpace color_space_;
    SkBitmap sk_bitmap_;

    DISALLOW_COPY_AND_ASSIGN(ScopedWriteLockSoftware);
  };

  class CC_EXPORT Fence : public base::RefCounted<Fence> {
   public:
    Fence() {}

    virtual void Set() = 0;
    virtual bool HasPassed() = 0;
    virtual void Wait() = 0;

   protected:
    friend class base::RefCounted<Fence>;
    virtual ~Fence() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(Fence);
  };

  class CC_EXPORT SynchronousFence : public ResourceProvider::Fence {
   public:
    explicit SynchronousFence(gpu::gles2::GLES2Interface* gl);

    // Overridden from Fence:
    void Set() override;
    bool HasPassed() override;
    void Wait() override;

    // Returns true if fence has been set but not yet synchornized.
    bool has_synchronized() const { return has_synchronized_; }

   private:
    ~SynchronousFence() override;

    void Synchronize();

    gpu::gles2::GLES2Interface* gl_;
    bool has_synchronized_;

    DISALLOW_COPY_AND_ASSIGN(SynchronousFence);
  };

  // For tests only! This prevents detecting uninitialized reads.
  // Use SetPixels or LockForWrite to allocate implicitly.
  void AllocateForTesting(viz::ResourceId id);

  // For tests only!
  void CreateForTesting(viz::ResourceId id);

  // Indicates if we can currently lock this resource for write.
  bool CanLockForWrite(viz::ResourceId id);

  // Indicates if this resource may be used for a hardware overlay plane.
  bool IsOverlayCandidate(viz::ResourceId id);

  // Return the format of the underlying buffer that can be used for scanout.
  gfx::BufferFormat GetBufferFormat(viz::ResourceId id);

#if defined(OS_ANDROID)
  // Indicates if this resource is backed by an Android SurfaceTexture, and thus
  // can't really be promoted to an overlay.
  bool IsBackedBySurfaceTexture(viz::ResourceId id);

  // Indicates if this resource wants to receive promotion hints.
  bool WantsPromotionHint(viz::ResourceId id);

  // Return the number of resources that request promotion hints.
  size_t CountPromotionHintRequestsForTesting();
#endif

  void WaitSyncToken(viz::ResourceId id);

  static GLint GetActiveTextureUnit(gpu::gles2::GLES2Interface* gl);

  static gpu::SyncToken GenerateSyncTokenHelper(gpu::gles2::GLES2Interface* gl);

  void ValidateResource(viz::ResourceId id) const;

  GLenum GetImageTextureTarget(gfx::BufferUsage usage,
                               viz::ResourceFormat format);

  // base::trace_event::MemoryDumpProvider implementation.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

  int tracing_id() const { return tracing_id_; }

#if defined(OS_TIZEN)
  void increase_frame_ack_count() { frame_ack_count_++; }
  int current_frame_ack_count() { return frame_ack_count_; }
  bool LockedForPreviousFrame(viz::ResourceId id);
  void FreeGrResources();
  bool GetRestoreResourceFilter () { return restore_resource_filter_; }
  void SetRestoreResourceFilter(bool restore) {
    restore_resource_filter_ = restore;
  }
#endif

 protected:
  struct Resource {
    enum Origin { INTERNAL, EXTERNAL, DELEGATED };
    enum SynchronizationState {
      // The LOCALLY_USED state is the state each resource defaults to when
      // constructed or modified or read. This state indicates that the
      // resource has not been properly synchronized and it would be an error
      // to send this resource to a parent, child, or client.
      LOCALLY_USED,

      // The NEEDS_WAIT state is the state that indicates a resource has been
      // modified but it also has an associated sync token assigned to it.
      // The sync token has not been waited on with the local context. When
      // a sync token arrives from an external resource (such as a child or
      // parent), it is automatically initialized as NEEDS_WAIT as well
      // since we still need to wait on it before the resource is synchronized
      // on the current context. It is an error to use the resource locally for
      // reading or writing if the resource is in this state.
      NEEDS_WAIT,

      // The SYNCHRONIZED state indicates that the resource has been properly
      // synchronized locally. This can either synchronized externally (such
      // as the case of software rasterized bitmaps), or synchronized
      // internally using a sync token that has been waited upon. In the
      // former case where the resource was synchronized externally, a
      // corresponding sync token will not exist. In the latter case which was
      // synchronized from the NEEDS_WAIT state, a corresponding sync token will
      // exist which is assocaited with the resource. This sync token is still
      // valid and still associated with the resource and can be passed as an
      // external resource for others to wait on.
      SYNCHRONIZED,
    };
    enum MipmapState { INVALID, GENERATE, VALID };

    Resource(GLuint texture_id,
             const gfx::Size& size,
             Origin origin,
             GLenum target,
             GLenum filter,
             TextureHint hint,
             ResourceType type,
             viz::ResourceFormat format);
    Resource(uint8_t* pixels,
             viz::SharedBitmap* bitmap,
             const gfx::Size& size,
             Origin origin,
             GLenum filter);
    Resource(const viz::SharedBitmapId& bitmap_id,
             const gfx::Size& size,
             Origin origin,
             GLenum filter);
    Resource(Resource&& other);
    ~Resource();

    bool needs_sync_token() const {
      return type != RESOURCE_TYPE_BITMAP &&
             synchronization_state_ == LOCALLY_USED;
    }

    SynchronizationState synchronization_state() const {
      return synchronization_state_;
    }

    const viz::TextureMailbox& mailbox() const { return mailbox_; }
    void SetMailbox(const viz::TextureMailbox& mailbox);

    void SetLocallyUsed();
    void SetSynchronized();
    void UpdateSyncToken(const gpu::SyncToken& sync_token);
    int8_t* GetSyncTokenData();
    void WaitSyncToken(gpu::gles2::GLES2Interface* sync_token);
    void SetGenerateMipmap();

    int child_id;
    viz::ResourceId id_in_child;
    GLuint gl_id;
    viz::ReleaseCallback release_callback;
    uint8_t* pixels;
    int lock_for_read_count;
    int imported_count;
    int exported_count;
    bool locked_for_write : 1;
    bool lost : 1;
    bool marked_for_deletion : 1;
    bool allocated : 1;
    bool read_lock_fences_enabled : 1;
    bool has_shared_bitmap_id : 1;
#if defined(TIZEN_TBM_SUPPORT)
    bool tbm_video : 1;
#endif
    bool is_overlay_candidate : 1;
#if defined(OS_ANDROID)
    // Indicates whether this resource may not be overlayed on Android, since
    // it's not backed by a SurfaceView.  This may be set in combination with
    // |is_overlay_candidate|, to find out if switching the resource to a
    // a SurfaceView would result in overlay promotion.  It's good to find this
    // out in advance, since one has no fallback path for displaying a
    // SurfaceView except via promoting it to an overlay.  Ideally, one _could_
    // promote SurfaceTexture via the overlay path, even if one ended up just
    // drawing a quad in the compositor.  However, for now, we use this flag to
    // refuse to promote so that the compositor will draw the quad.
    bool is_backed_by_surface_texture : 1;
    // Indicates that this resource would like a promotion hint.
    bool wants_promotion_hint : 1;
#endif
    scoped_refptr<Fence> read_lock_fence;
    gfx::Size size;
    Origin origin;
    GLenum target;
    // TODO(skyostil): Use a separate sampler object for filter state.
    GLenum original_filter;
    GLenum filter;
    GLenum min_filter;
    GLuint image_id;
    TextureHint hint;
    ResourceType type;
    // GpuMemoryBuffer resource allocation needs to know how the resource will
    // be used.
    gfx::BufferUsage usage;
    // This is the the actual format of the underlaying GpuMemoryBuffer, if any,
    // and might not correspond to viz::ResourceFormat. This format is needed to
    // scanout the buffer as HW overlay.
    gfx::BufferFormat buffer_format;
    // Resource format is the format as seen from the compositor and might not
    // correspond to buffer_format (e.g: A resouce that was created from a YUV
    // buffer could be seen as RGB from the compositor/GL.)
    viz::ResourceFormat format;
    viz::SharedBitmapId shared_bitmap_id;
    viz::SharedBitmap* shared_bitmap;
    std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer;
    gfx::ColorSpace color_space;
    MipmapState mipmap_state = INVALID;
#if defined(OS_TIZEN)
    int last_used_frame_ack_index = 0;
    bool needs_reset_filter : 1;
#endif

   private:
    SynchronizationState synchronization_state_ = SYNCHRONIZED;
    viz::TextureMailbox mailbox_;

    DISALLOW_COPY_AND_ASSIGN(Resource);
  };
  using ResourceMap = std::unordered_map<viz::ResourceId, Resource>;

  Resource* InsertResource(viz::ResourceId id, Resource resource);
  Resource* GetResource(viz::ResourceId id);
  Resource* LockForWrite(viz::ResourceId id);
  void UnlockForWrite(Resource* resource);

  void PopulateSkBitmapWithResource(SkBitmap* sk_bitmap,
                                    const Resource* resource);

  void CreateAndBindImage(Resource* resource);

  // Binds the given GL resource to a texture target for sampling using the
  // specified filter for both minification and magnification. Returns the
  // texture target used. The resource must be locked for reading.
  GLenum BindForSampling(viz::ResourceId resource_id,
                         GLenum unit,
                         GLenum filter);

  gfx::ColorSpace GetResourceColorSpaceForRaster(
      const Resource* resource) const;

  enum DeleteStyle {
    NORMAL,
    FOR_SHUTDOWN,
  };
  void DeleteResourceInternal(ResourceMap::iterator it, DeleteStyle style);

  void CreateMailbox(Resource* resource);

  bool ReadLockFenceHasPassed(const Resource* resource) {
    return !resource->read_lock_fence.get() ||
           resource->read_lock_fence->HasPassed();
  }

  // Returns null if we do not have a viz::ContextProvider.
  gpu::gles2::GLES2Interface* ContextGL() const;

  // Holds const settings for the ResourceProvider. Never changed after init.
  struct Settings {
    Settings(viz::ContextProvider* compositor_context_provider,
             bool delegated_sync_points_needed,
             const viz::ResourceSettings& resource_settings);

    int max_texture_size = 0;
    bool use_texture_storage_ext = false;
    bool use_texture_format_bgra = false;
    bool use_texture_usage_hint = false;
    bool use_texture_npot = false;
    bool use_sync_query = false;
    ResourceType default_resource_type = RESOURCE_TYPE_GL_TEXTURE;
    viz::ResourceFormat yuv_resource_format = viz::LUMINANCE_8;
    viz::ResourceFormat yuv_highbit_resource_format = viz::LUMINANCE_8;
    viz::ResourceFormat best_texture_format = viz::RGBA_8888;
    viz::ResourceFormat best_render_buffer_format = viz::RGBA_8888;
    bool delegated_sync_points_required = false;
  } const settings_;

  ResourceMap resources_;

  // Keep track of whether deleted resources should be batched up or returned
  // immediately.
  bool batch_return_resources_ = false;
  // Maps from a child id to the set of resources to be returned to it.
  base::small_map<std::map<int, ResourceIdArray>> batched_returning_resources_;

  viz::ContextProvider* compositor_context_provider_;
  viz::SharedBitmapManager* shared_bitmap_manager_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
  viz::ResourceId next_id_;
  int next_child_;
#if defined(OS_TIZEN)
  int frame_ack_count_ = 0;
#endif

  bool lost_context_provider_;

  THREAD_CHECKER(thread_checker_);

#if defined(OS_ANDROID)
  // Set of resource Ids that would like to be notified about promotion hints.
  viz::ResourceIdSet wants_promotion_hints_set_;
#endif

 private:
  viz::ResourceId CreateGpuResource(const gfx::Size& size,
                                    TextureHint hint,
                                    ResourceType type,
                                    viz::ResourceFormat format,
                                    gfx::BufferUsage usage,
                                    const gfx::ColorSpace& color_space);
  viz::ResourceId CreateBitmapResource(const gfx::Size& size,
                                       const gfx::ColorSpace& color_space);

  void CreateTexture(Resource* resource);

  bool IsGLContextLost() const;

  std::unique_ptr<TextureIdAllocator> texture_id_allocator_;
  viz::BufferToTextureTargetMap buffer_to_texture_target_map_;

  // A process-unique ID used for disambiguating memory dumps from different
  // resource providers.
  int tracing_id_;
#if defined(OS_TIZEN)
  bool restore_resource_filter_ = true;
#endif

  DISALLOW_COPY_AND_ASSIGN(ResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_RESOURCE_PROVIDER_H_
