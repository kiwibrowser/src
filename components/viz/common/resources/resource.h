// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_H_

#include <memory>

#include "build/build_config.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/resources/release_callback.h"
#include "components/viz/common/resources/resource_fence.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/resources/resource_type.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace gfx {
class GpuMemoryBuffer;
}

namespace viz {
namespace internal {

// The data structure used to track state of Gpu and Software-based
// resources both in the client and the service, for resources transferred
// between the two. This is an implementation detail of the resource tracking
// for client and service libraries and should not be used directly from
// external client code.
struct VIZ_COMMON_EXPORT Resource {
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
    // exist which is associated with the resource. This sync token is still
    // valid and still associated with the resource and can be passed as an
    // external resource for others to wait on.
    SYNCHRONIZED,
  };

  Resource(const gfx::Size& size,
           ResourceType type,
           ResourceFormat format,
           const gfx::ColorSpace& color_space);

  Resource(Resource&& other);

  ~Resource();

  Resource& operator=(Resource&& other);

  bool is_gpu_resource_type() const { return type != ResourceType::kBitmap; }

  bool needs_sync_token() const {
    return type != ResourceType::kBitmap &&
           synchronization_state_ == LOCALLY_USED;
  }

  const gpu::SyncToken& sync_token() const { return sync_token_; }

  SynchronizationState synchronization_state() const {
    return synchronization_state_;
  }

  void SetSharedBitmap(SharedBitmap* bitmap);

  void SetLocallyUsed();
  void SetSynchronized();
  void UpdateSyncToken(const gpu::SyncToken& sync_token);
  // If true the texture-backed or GpuMemoryBuffer-backed resource needs its
  // SyncToken waited on in order to be synchronized for use.
  bool ShouldWaitSyncToken() const;
  int8_t* GetSyncTokenData();

  // Bitfield flags. ======
  // When true, the resource is currently being used externally.
  bool locked_for_external_use : 1;
  // When the resource should be deleted until it is actually reaped.
  bool marked_for_deletion : 1;
  // Tracks if a gpu fence needs to be used for reading a GpuMemoryBuffer-
  // backed or texture-backed resource.
  bool read_lock_fences_enabled : 1;
  // True if the software-backed resource is in shared memory, in which case
  // |shared_bitmap_id| will be valid.
  bool has_shared_bitmap_id : 1;
  // When true, the resource should be considered for being displayed in an
  // overlay.
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

  // In the service, this is the id of the client the resource comes from.
  int child_id = 0;
  // In the service, this is the id of the resource in the client's namespace.
  ResourceId id_in_child = 0;
  // Texture id for texture-backed resources.
  GLuint gl_id = 0;
  // The mailbox associated with resources received from the client to the
  // service. The mailbox has the IPC-capable data for sharing the resource
  // backing between modules/GL contexts/processes.
  gpu::Mailbox mailbox;
  // Non-owning pointer to a software-backed resource when mapped.
  uint8_t* pixels = nullptr;
  // Reference-counts to know when a resource can be released through the
  // |release_callback| after it is deleted, and for verifying correct use
  // of the resource.
  int lock_for_read_count = 0;
  int imported_count = 0;
  // A fence used for accessing a GpuMemoryBuffer-backed or texture-backed
  // resource for reading, that ensures any writing done to the resource has
  // been completed. This is implemented and used to implement transferring
  // ownership of the resource from the client to the service, and in the GL
  // drawing code before reading from the texture.
  scoped_refptr<ResourceFence> read_lock_fence;
  // Size of the resource in pixels.
  gfx::Size size;
  // The texture target for GpuMemoryBuffer- and texture-backed resources.
  GLenum target = GL_TEXTURE_2D;
  // The min/mag filter of the resource when it was given to/created by the
  // ResourceProvider, for texture-backed resources. Used to restore
  // the filter before releasing the resource. Not used for GpuMemoryBuffer-
  // backed resources as they are always internally created, so not released.
  // TODO(skyostil): Use a separate sampler object for filter state.
  GLenum original_filter = GL_LINEAR;
  // The current mag filter for GpuMemoryBuffer- and texture-backed resources.
  GLenum filter = GL_LINEAR;
  // The current min filter for GpuMemoryBuffer- and texture-backed resources.
  GLenum min_filter = GL_LINEAR;
  // The type of backing for the resource (such as gpu vs software).
  ResourceType type = ResourceType::kBitmap;
  // This is the the actual format of the underlying GpuMemoryBuffer, if any,
  // and might not correspond to ResourceFormat. This format is needed to
  // allocate the GpuMemoryBuffer and scanout the buffer as a hardware overlay.
  gfx::BufferFormat buffer_format = gfx::BufferFormat::RGBA_8888;
  // The format as seen from the compositor and might not correspond to
  // buffer_format (e.g: A resouce that was created from a YUV buffer could be
  // seen as RGB from the compositor/GL). This is used to derive the GL texture
  // format for texture-backed resources, the image format for GpuMemoryBuffer-
  // backed resources, or the SkColorType used for software-backed resources.
  ResourceFormat format = ResourceFormat::RGBA_8888;
  // The name of the shared memory for software-backed resources, but not
  // present if the resource isn't shared memory.
  SharedBitmapId shared_bitmap_id;
  // A pointer to the shared memory structure for software-backed resources, but
  // not present if the resources isn't shared memory.
  SharedBitmap* shared_bitmap = nullptr;
  // Ownership of |shared_bitmap| for when it is created internally.
  std::unique_ptr<SharedBitmap> owned_shared_bitmap;
  // The color space for all resource types, to control how the resource should
  // be drawn to output device.
  gfx::ColorSpace color_space;

 private:
  // Tracks if a sync token needs to be waited on before using the resource.
  SynchronizationState synchronization_state_ = SYNCHRONIZED;
  // A SyncToken associated with a texture-backed or GpuMemoryBuffer-backed
  // resource. It is given from a child to the service, and waited on in order
  // to use the resource, and this is tracked by the |synchronization_state_|.
  gpu::SyncToken sync_token_;

  DISALLOW_COPY_AND_ASSIGN(Resource);
};

}  // namespace internal
}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_H_
