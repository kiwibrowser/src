// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/layer_tree_resource_provider.h"

#include "base/bits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "components/viz/common/resources/returned_resource.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "third_party/skia/include/core/SkCanvas.h"

using gpu::gles2::GLES2Interface;

namespace cc {

struct LayerTreeResourceProvider::ImportedResource {
  viz::TransferableResource resource;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;
  int exported_count = 0;
  bool marked_for_deletion = false;

  gpu::SyncToken returned_sync_token;
  bool returned_lost = false;

  ImportedResource(viz::ResourceId id,
                   const viz::TransferableResource& resource,
                   std::unique_ptr<viz::SingleReleaseCallback> release_callback)
      : resource(resource),
        release_callback(std::move(release_callback)),
        // If the resource is immediately deleted, it returns the same SyncToken
        // it came with. The client may need to wait on that before deleting the
        // backing or reusing it.
        returned_sync_token(resource.mailbox_holder.sync_token) {
    // Replace the |resource| id with the local id from this
    // LayerTreeResourceProvider.
    this->resource.id = id;
  }
  ~ImportedResource() = default;

  ImportedResource(ImportedResource&&) = default;
  ImportedResource& operator=(ImportedResource&&) = default;
};

LayerTreeResourceProvider::LayerTreeResourceProvider(
    viz::ContextProvider* compositor_context_provider,
    bool delegated_sync_points_required)
    : delegated_sync_points_required_(delegated_sync_points_required),
      compositor_context_provider_(compositor_context_provider) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

LayerTreeResourceProvider::~LayerTreeResourceProvider() {
  for (auto& pair : imported_resources_) {
    ImportedResource& imported = pair.second;
    // If the resource is exported we can't report when it can be used again
    // once this class is destroyed, so consider the resource lost.
    bool is_lost = imported.exported_count || imported.returned_lost;
    imported.release_callback->Run(imported.returned_sync_token, is_lost);
  }
}

gpu::SyncToken LayerTreeResourceProvider::GenerateSyncTokenHelper(
    gpu::gles2::GLES2Interface* gl) {
  DCHECK(gl);
  gpu::SyncToken sync_token;
  gl->GenUnverifiedSyncTokenCHROMIUM(sync_token.GetData());
  DCHECK(sync_token.HasData() ||
         gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR);
  return sync_token;
}

gpu::SyncToken LayerTreeResourceProvider::GenerateSyncTokenHelper(
    gpu::raster::RasterInterface* ri) {
  DCHECK(ri);
  gpu::SyncToken sync_token;
  ri->GenUnverifiedSyncTokenCHROMIUM(sync_token.GetData());
  DCHECK(sync_token.HasData() ||
         ri->GetGraphicsResetStatusKHR() != GL_NO_ERROR);
  return sync_token;
}

void LayerTreeResourceProvider::PrepareSendToParent(
    const std::vector<viz::ResourceId>& export_ids,
    std::vector<viz::TransferableResource>* list,
    viz::ContextProvider* context_provider) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // This function goes through the array multiple times, store the resources
  // as pointers so we don't have to look up the resource id multiple times.
  // Make sure the maps do not change while these vectors are alive or they
  // will become invalid.
  std::vector<ImportedResource*> imports;
  imports.reserve(export_ids.size());
  for (const viz::ResourceId id : export_ids) {
    auto it = imported_resources_.find(id);
    DCHECK(it != imported_resources_.end());
    imports.push_back(&it->second);
  }

  // Lazily create any mailboxes and verify all unverified sync tokens.
  std::vector<GLbyte*> unverified_sync_tokens;
  if (delegated_sync_points_required_) {
    for (ImportedResource* imported : imports) {
      if (!imported->resource.is_software &&
          !imported->resource.mailbox_holder.sync_token.verified_flush()) {
        unverified_sync_tokens.push_back(
            imported->resource.mailbox_holder.sync_token.GetData());
      }
    }
  }

  if (!unverified_sync_tokens.empty()) {
    DCHECK(delegated_sync_points_required_);
    DCHECK(context_provider);
    context_provider->ContextGL()->VerifySyncTokensCHROMIUM(
        unverified_sync_tokens.data(), unverified_sync_tokens.size());
  }

  for (ImportedResource* imported : imports) {
    list->push_back(imported->resource);
    imported->exported_count++;
  }
}

void LayerTreeResourceProvider::ReceiveReturnsFromParent(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (const viz::ReturnedResource& returned : resources) {
    viz::ResourceId local_id = returned.id;

    auto import_it = imported_resources_.find(local_id);
    DCHECK(import_it != imported_resources_.end());
    ImportedResource& imported = import_it->second;

    DCHECK_GE(imported.exported_count, returned.count);
    imported.exported_count -= returned.count;
    imported.returned_lost |= returned.lost;

    if (imported.exported_count)
      continue;

    if (returned.sync_token.HasData()) {
      DCHECK(!imported.resource.is_software);
      imported.returned_sync_token = returned.sync_token;
    }

    if (imported.marked_for_deletion) {
      imported.release_callback->Run(imported.returned_sync_token,
                                     imported.returned_lost);
      imported_resources_.erase(import_it);
    }
  }
}

viz::ResourceId LayerTreeResourceProvider::ImportResource(
    const viz::TransferableResource& resource,
    std::unique_ptr<viz::SingleReleaseCallback> release_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  viz::ResourceId id = next_id_++;
  auto result = imported_resources_.emplace(
      id, ImportedResource(id, resource, std::move(release_callback)));
  DCHECK(result.second);  // If false, the id was already in the map.
  return id;
}

void LayerTreeResourceProvider::RemoveImportedResource(viz::ResourceId id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = imported_resources_.find(id);
  DCHECK(it != imported_resources_.end());
  ImportedResource& imported = it->second;
  imported.marked_for_deletion = true;
  if (imported.exported_count == 0) {
    imported.release_callback->Run(imported.returned_sync_token,
                                   imported.returned_lost);
    imported_resources_.erase(it);
  }
}

bool LayerTreeResourceProvider::IsTextureFormatSupported(
    viz::ResourceFormat format) const {
  gpu::Capabilities caps;
  if (compositor_context_provider_)
    caps = compositor_context_provider_->ContextCapabilities();

  switch (format) {
    case viz::ALPHA_8:
    case viz::RGBA_4444:
    case viz::RGBA_8888:
    case viz::RGB_565:
    case viz::LUMINANCE_8:
      return true;
    case viz::BGRA_8888:
      return caps.texture_format_bgra8888;
    case viz::ETC1:
      return caps.texture_format_etc1;
    case viz::RED_8:
      return caps.texture_rg;
    case viz::R16_EXT:
      return caps.texture_norm16;
    case viz::LUMINANCE_F16:
    case viz::RGBA_F16:
      return caps.texture_half_float_linear;
  }

  NOTREACHED();
  return false;
}

bool LayerTreeResourceProvider::IsRenderBufferFormatSupported(
    viz::ResourceFormat format) const {
  gpu::Capabilities caps;
  if (compositor_context_provider_)
    caps = compositor_context_provider_->ContextCapabilities();

  switch (format) {
    case viz::RGBA_4444:
    case viz::RGBA_8888:
    case viz::RGB_565:
      return true;
    case viz::BGRA_8888:
      return caps.render_buffer_format_bgra8888;
    case viz::RGBA_F16:
      // TODO(ccameron): This will always return false on pixel tests, which
      // makes it un-test-able until we upgrade Mesa.
      // https://crbug.com/687720
      return caps.texture_half_float_linear &&
             caps.color_buffer_half_float_rgba;
    case viz::LUMINANCE_8:
    case viz::ALPHA_8:
    case viz::RED_8:
    case viz::ETC1:
    case viz::LUMINANCE_F16:
    case viz::R16_EXT:
      // We don't currently render into these formats. If we need to render into
      // these eventually, we should expand this logic.
      return false;
  }

  NOTREACHED();
  return false;
}

LayerTreeResourceProvider::ScopedSkSurface::ScopedSkSurface(
    GrContext* gr_context,
    GLuint texture_id,
    GLenum texture_target,
    const gfx::Size& size,
    viz::ResourceFormat format,
    bool can_use_lcd_text,
    int msaa_sample_count) {
  GrGLTextureInfo texture_info;
  texture_info.fID = texture_id;
  texture_info.fTarget = texture_target;
  texture_info.fFormat = TextureStorageFormat(format);
  GrBackendTexture backend_texture(size.width(), size.height(),
                                   GrMipMapped::kNo, texture_info);
  SkSurfaceProps surface_props = ComputeSurfaceProps(can_use_lcd_text);
  // This type is used only for gpu raster, which implies gpu compositing.
  bool gpu_compositing = true;
  surface_ = SkSurface::MakeFromBackendTextureAsRenderTarget(
      gr_context, backend_texture, kTopLeft_GrSurfaceOrigin, msaa_sample_count,
      ResourceFormatToClosestSkColorType(gpu_compositing, format), nullptr,
      &surface_props);
}

LayerTreeResourceProvider::ScopedSkSurface::~ScopedSkSurface() {
  if (surface_)
    surface_->prepareForExternalIO();
}

SkSurfaceProps LayerTreeResourceProvider::ScopedSkSurface::ComputeSurfaceProps(
    bool can_use_lcd_text) {
  uint32_t flags = 0;
  // Use unknown pixel geometry to disable LCD text.
  SkSurfaceProps surface_props(flags, kUnknown_SkPixelGeometry);
  if (can_use_lcd_text) {
    // LegacyFontHost will get LCD text and skia figures out what type to use.
    surface_props =
        SkSurfaceProps(flags, SkSurfaceProps::kLegacyFontHost_InitType);
  }
  return surface_props;
}

void LayerTreeResourceProvider::ValidateResource(viz::ResourceId id) const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(id);
  DCHECK(imported_resources_.find(id) != imported_resources_.end());
}

bool LayerTreeResourceProvider::InUseByConsumer(viz::ResourceId id) {
  auto it = imported_resources_.find(id);
  DCHECK(it != imported_resources_.end());
  ImportedResource& imported = it->second;
  return imported.exported_count > 0 || imported.returned_lost;
}

}  // namespace cc
