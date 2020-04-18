// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_LAYER_TREE_RESOURCE_PROVIDER_H_
#define CC_RESOURCES_LAYER_TREE_RESOURCE_PROVIDER_H_

#include <vector>

#include "base/threading/thread_checker.h"
#include "cc/cc_export.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/resources/release_callback.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/resources/resource_settings.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
namespace raster {
class RasterInterface;
}
}  // namespace gpu

namespace viz {
class ContextProvider;
}  // namespace viz

namespace cc {

// This class is not thread-safe and can only be called from the thread it was
// created on (in practice, the impl thread).
class CC_EXPORT LayerTreeResourceProvider {
 public:
  LayerTreeResourceProvider(viz::ContextProvider* compositor_context_provider,
                            bool delegated_sync_points_required);
  ~LayerTreeResourceProvider();

  static gpu::SyncToken GenerateSyncTokenHelper(gpu::gles2::GLES2Interface* gl);
  static gpu::SyncToken GenerateSyncTokenHelper(
      gpu::raster::RasterInterface* ri);

  // Prepares resources to be transfered to the parent, moving them to
  // mailboxes and serializing meta-data into TransferableResources.
  // Resources are not removed from the ResourceProvider, but are marked as
  // "in use".
  void PrepareSendToParent(
      const std::vector<viz::ResourceId>& resource_ids,
      std::vector<viz::TransferableResource>* transferable_resources,
      viz::ContextProvider* context_provider);

  // Receives resources from the parent, moving them from mailboxes. ResourceIds
  // passed are in the child namespace.
  // NOTE: if the sync_token is set on any viz::TransferableResource, this will
  // wait on it.
  void ReceiveReturnsFromParent(
      const std::vector<viz::ReturnedResource>& transferable_resources);

  // Receives a resource from an external client that can be used in compositor
  // frames, via the returned ResourceId.
  viz::ResourceId ImportResource(const viz::TransferableResource&,
                                 std::unique_ptr<viz::SingleReleaseCallback>);
  // Removes an imported resource, which will call the ReleaseCallback given
  // originally, once the resource is no longer in use by any compositor frame.
  void RemoveImportedResource(viz::ResourceId);

  // Verify that the ResourceId is valid and is known to this class, for debug
  // checks.
  void ValidateResource(viz::ResourceId id) const;

  // Checks whether a resource is in use by a consumer.
  bool InUseByConsumer(viz::ResourceId id);

  bool IsTextureFormatSupported(viz::ResourceFormat format) const;

  // Returns true if the provided |format| can be used as a render buffer.
  // Note that render buffer support implies texture support.
  bool IsRenderBufferFormatSupported(viz::ResourceFormat format) const;

  bool IsSoftware() const { return !compositor_context_provider_; }

  class CC_EXPORT ScopedSkSurface {
   public:
    ScopedSkSurface(GrContext* gr_context,
                    GLuint texture_id,
                    GLenum texture_target,
                    const gfx::Size& size,
                    viz::ResourceFormat format,
                    bool can_use_lcd_text,
                    int msaa_sample_count);
    ~ScopedSkSurface();

    SkSurface* surface() const { return surface_.get(); }

    static SkSurfaceProps ComputeSurfaceProps(bool can_use_lcd_text);

   private:
    sk_sp<SkSurface> surface_;

    DISALLOW_COPY_AND_ASSIGN(ScopedSkSurface);
  };

 private:
  struct ImportedResource;

  THREAD_CHECKER(thread_checker_);
  const bool delegated_sync_points_required_;
  viz::ContextProvider* const compositor_context_provider_;

  base::flat_map<viz::ResourceId, ImportedResource> imported_resources_;
  // The ResourceIds in LayerTreeResourceProvider start from 1 to avoid
  // conflicts with id from viz::DisplayResourceProvider.
  viz::ResourceId next_id_ = 1;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_LAYER_TREE_RESOURCE_PROVIDER_H_
