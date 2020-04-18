// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/compositor_layer_resource.h"

#include <limits>

#include "base/logging.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "ppapi/proxy/compositor_resource.h"
#include "ppapi/shared_impl/ppb_graphics_3d_shared.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_graphics_3d_api.h"
#include "ppapi/thunk/ppb_image_data_api.h"

using gpu::gles2::GLES2Implementation;
using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_ImageData_API;
using ppapi::thunk::PPB_Graphics3D_API;

namespace ppapi {
namespace proxy {

namespace {

float clamp(float value) {
  return std::min(std::max(value, 0.0f), 1.0f);
}

void OnTextureReleased(const ScopedPPResource& layer,
                       const ScopedPPResource& context,
                       uint32_t texture,
                       const scoped_refptr<TrackedCallback>& release_callback,
                       int32_t result,
                       const gpu::SyncToken& sync_token,
                       bool is_lost) {
  if (!TrackedCallback::IsPending(release_callback))
    return;

  if (result != PP_OK) {
    release_callback->Run(result);
    return;
  }

  do {
    if (!sync_token.HasData())
      break;

    EnterResourceNoLock<PPB_Graphics3D_API> enter(context.get(), true);
    if (enter.failed())
      break;

    PPB_Graphics3D_Shared* graphics =
        static_cast<PPB_Graphics3D_Shared*>(enter.object());

    GLES2Implementation* gl = graphics->gles2_impl();
    gl->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  } while (false);

  release_callback->Run(is_lost ? PP_ERROR_FAILED : PP_OK);
}

void OnImageReleased(const ScopedPPResource& layer,
                     const ScopedPPResource& image,
                     const scoped_refptr<TrackedCallback>& release_callback,
                     int32_t result,
                     const gpu::SyncToken& sync_token,
                     bool is_lost) {
  if (!TrackedCallback::IsPending(release_callback))
    return;
  release_callback->Run(result);
}

}  // namespace

CompositorLayerResource::CompositorLayerResource(
    Connection connection,
    PP_Instance instance,
    const CompositorResource* compositor)
    : PluginResource(connection, instance),
      compositor_(compositor),
      source_size_(PP_MakeFloatSize(0.0f, 0.0f)) {
}

CompositorLayerResource::~CompositorLayerResource() {
  DCHECK(!compositor_);
  DCHECK(release_callback_.is_null());
}

thunk::PPB_CompositorLayer_API*
CompositorLayerResource::AsPPB_CompositorLayer_API() {
  return this;
}

int32_t CompositorLayerResource::SetColor(float red,
                                          float green,
                                          float blue,
                                          float alpha,
                                          const PP_Size* size) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  if (!SetType(TYPE_COLOR))
    return PP_ERROR_BADARGUMENT;
  DCHECK(data_.color);

  if (!size)
    return PP_ERROR_BADARGUMENT;

  data_.color->red = clamp(red);
  data_.color->green = clamp(green);
  data_.color->blue = clamp(blue);
  data_.color->alpha = clamp(alpha);
  data_.common.size = *size;

  return PP_OK;
}

int32_t CompositorLayerResource::SetTexture0_1(
    PP_Resource context,
    uint32_t texture,
    const PP_Size* size,
    const scoped_refptr<TrackedCallback>& release_callback) {
  return SetTexture(context, GL_TEXTURE_2D, texture, size, release_callback);
}

int32_t CompositorLayerResource::SetTexture(
    PP_Resource context,
    uint32_t target,
    uint32_t texture,
    const PP_Size* size,
    const scoped_refptr<TrackedCallback>& release_callback) {
  int32_t rv = CheckForSetTextureAndImage(TYPE_TEXTURE, release_callback);
  if (rv != PP_OK)
    return rv;
  DCHECK(data_.texture);

  EnterResourceNoLock<PPB_Graphics3D_API> enter(context, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;

  if (target != GL_TEXTURE_2D &&
      target != GL_TEXTURE_EXTERNAL_OES &&
      target != GL_TEXTURE_RECTANGLE_ARB) {
    return PP_ERROR_BADARGUMENT;
  }

  if (!size || size->width <= 0 || size->height <= 0)
    return PP_ERROR_BADARGUMENT;

  PPB_Graphics3D_Shared* graphics =
      static_cast<PPB_Graphics3D_Shared*>(enter.object());

  GLES2Implementation* gl = graphics->gles2_impl();

  // Generate a Mailbox for the texture.
  gl->GenMailboxCHROMIUM(
      reinterpret_cast<GLbyte*>(data_.texture->mailbox.name));
  gl->ProduceTextureDirectCHROMIUM(
      texture, reinterpret_cast<const GLbyte*>(data_.texture->mailbox.name));

  // Set the source size to (1, 1). It will be used to verify the source_rect
  // passed to SetSourceRect().
  source_size_ = PP_MakeFloatSize(1.0f, 1.0f);

  data_.common.size = *size;
  data_.common.resource_id = compositor_->GenerateResourceId();
  data_.texture->target = target;
  data_.texture->source_rect.point = PP_MakeFloatPoint(0.0f, 0.0f);
  data_.texture->source_rect.size = source_size_;

  gl->GenSyncTokenCHROMIUM(data_.texture->sync_token.GetData());

  // If the PP_Resource of this layer is released by the plugin, the
  // release_callback will be aborted immediately, but the texture or image
  // in this layer may still being used by chromium compositor. So we have to
  // use ScopedPPResource to keep this resource alive until the texture or image
  // is released by the chromium compositor.
  release_callback_ = base::Bind(
      &OnTextureReleased,
      ScopedPPResource(pp_resource()), // Keep layer alive.
      ScopedPPResource(context), // Keep context alive
      texture,
      release_callback);

  return PP_OK_COMPLETIONPENDING;
}

int32_t CompositorLayerResource::SetImage(
    PP_Resource image_data,
    const PP_Size* size,
    const scoped_refptr<TrackedCallback>& release_callback) {
  int32_t rv = CheckForSetTextureAndImage(TYPE_IMAGE, release_callback);
  if (rv != PP_OK)
    return rv;
  DCHECK(data_.image);

  EnterResourceNoLock<PPB_ImageData_API> enter(image_data, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;

  PP_ImageDataDesc desc;
  if (!enter.object()->Describe(&desc))
    return PP_ERROR_BADARGUMENT;

  // TODO(penghuang): Support image which width * 4 != stride.
  if (desc.size.width * 4 != desc.stride)
    return PP_ERROR_BADARGUMENT;

  // TODO(penghuang): Support all formats.
  if (desc.format != PP_IMAGEDATAFORMAT_RGBA_PREMUL)
    return PP_ERROR_BADARGUMENT;

  if (size && (size->width <= 0 || size->height <= 0))
    return PP_ERROR_BADARGUMENT;

  // Set the source size to image's size. It will be used to verify
  // the source_rect passed to SetSourceRect().
  source_size_ = PP_MakeFloatSize(desc.size.width, desc.size.height);

  data_.common.size = size ? *size : desc.size;
  data_.common.resource_id = compositor_->GenerateResourceId();
  data_.image->resource = enter.resource()->host_resource().host_resource();
  data_.image->source_rect.point = PP_MakeFloatPoint(0.0f, 0.0f);
  data_.image->source_rect.size = source_size_;

  // If the PP_Resource of this layer is released by the plugin, the
  // release_callback will be aborted immediately, but the texture or image
  // in this layer may still being used by chromium compositor. So we have to
  // use ScopedPPResource to keep this resource alive until the texture or image
  // is released by the chromium compositor.
  release_callback_ = base::Bind(
      &OnImageReleased,
      ScopedPPResource(pp_resource()), // Keep layer alive.
      ScopedPPResource(image_data), // Keep image_data alive.
      release_callback);

  return PP_OK_COMPLETIONPENDING;
}

int32_t CompositorLayerResource::SetClipRect(const PP_Rect* rect) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  data_.common.clip_rect = rect ? *rect : PP_MakeRectFromXYWH(0, 0, 0, 0);
  return PP_OK;
}

int32_t CompositorLayerResource::SetTransform(const float matrix[16]) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  std::copy(matrix, matrix + 16, data_.common.transform.matrix);
  return PP_OK;
}

int32_t CompositorLayerResource::SetOpacity(float opacity) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  data_.common.opacity = clamp(opacity);
  return PP_OK;
}

int32_t CompositorLayerResource::SetBlendMode(PP_BlendMode mode) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  switch (mode) {
    case PP_BLENDMODE_NONE:
    case PP_BLENDMODE_SRC_OVER:
      data_.common.blend_mode = mode;
      return PP_OK;
  }
  return PP_ERROR_BADARGUMENT;
}

int32_t CompositorLayerResource::SetSourceRect(
    const PP_FloatRect* rect) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  const float kEpsilon = std::numeric_limits<float>::epsilon();
  if (!rect ||
      rect->point.x < -kEpsilon ||
      rect->point.y < -kEpsilon ||
      rect->point.x + rect->size.width > source_size_.width + kEpsilon ||
      rect->point.y + rect->size.height > source_size_.height + kEpsilon) {
    return PP_ERROR_BADARGUMENT;
  }

  if (data_.texture) {
    data_.texture->source_rect = *rect;
    return PP_OK;
  }
  if (data_.image) {
    data_.image->source_rect = *rect;
    return PP_OK;
  }
  return PP_ERROR_BADARGUMENT;
}

int32_t CompositorLayerResource::SetPremultipliedAlpha(PP_Bool premult) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  if (data_.texture) {
    data_.texture->premult_alpha = PP_ToBool(premult);
    return PP_OK;
  }
  return PP_ERROR_BADARGUMENT;
}

bool CompositorLayerResource::SetType(LayerType type) {
  if (type == TYPE_COLOR) {
    if (data_.is_null())
      data_.color.reset(new CompositorLayerData::ColorLayer());
    return !!data_.color;
  }

  if (type == TYPE_TEXTURE) {
    if (data_.is_null())
      data_.texture.reset(new CompositorLayerData::TextureLayer());
    return !!data_.texture;
  }

  if (type == TYPE_IMAGE) {
    if (data_.is_null())
      data_.image.reset(new CompositorLayerData::ImageLayer());
    return !!data_.image;
  }

  // Should not be reached.
  DCHECK(false);
  return false;
}

int32_t CompositorLayerResource::CheckForSetTextureAndImage(
    LayerType type,
    const scoped_refptr<TrackedCallback>& release_callback) {
  if (!compositor_)
    return PP_ERROR_BADRESOURCE;

  if (compositor_->IsInProgress())
    return PP_ERROR_INPROGRESS;

  if (!SetType(type))
    return PP_ERROR_BADARGUMENT;

  // The layer's image has been set and it is not committed.
  if (!release_callback_.is_null())
    return PP_ERROR_INPROGRESS;

  // Do not allow using a block callback as a release callback.
  if (release_callback->is_blocking())
    return PP_ERROR_BADARGUMENT;

  return PP_OK;
}

}  // namespace proxy
}  // namespace ppapi
