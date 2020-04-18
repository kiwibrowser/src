// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_METADATA_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_METADATA_H_

#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// Metadata for a Resoource. It is used by SkiaRenderer to get resource metedata
// from DisplayResourceProvider, and make a promise SkImage from it.
struct VIZ_COMMON_EXPORT ResourceMetadata {
  ResourceMetadata();
  ResourceMetadata(ResourceMetadata&& other);
  ~ResourceMetadata();
  ResourceMetadata& operator=(ResourceMetadata&& other);

  // A mailbox for the resource texture.
  gpu::Mailbox mailbox;

  // The backend format for the resource texture. It includes sized texture
  // format and texture target.
  GrBackendFormat backend_format;

  // The resource size.
  gfx::Size size;

  // the mipmap of the resource texture.
  GrMipMapped mip_mapped = GrMipMapped::kNo;

  // The origin type for the resource texture.
  GrSurfaceOrigin origin = kTopLeft_GrSurfaceOrigin;

  // The color type for creating SkImage from the resource texture.
  SkColorType color_type = kUnknown_SkColorType;

  // The alpha type for the resource texture.
  SkAlphaType alpha_type = kUnknown_SkAlphaType;

  // The color space for the resource texture. It could be null.
  sk_sp<SkColorSpace> color_space;

  // The sync token for the resource texture.
  gpu::SyncToken sync_token;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_METADATA_H_
