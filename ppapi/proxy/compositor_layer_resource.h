// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_COMPOSITOR_LAYER_RESOURCE_H_
#define PPAPI_PROXY_COMPOSITOR_LAYER_RESOURCE_H_

#include <stdint.h>

#include "base/macros.h"
#include "ppapi/c/ppb_compositor_layer.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/compositor_layer_data.h"
#include "ppapi/shared_impl/scoped_pp_resource.h"
#include "ppapi/thunk/ppb_compositor_layer_api.h"

namespace gpu {
struct SyncToken;
}

namespace ppapi {
namespace proxy {

class CompositorResource;

class PPAPI_PROXY_EXPORT CompositorLayerResource
    : public PluginResource,
      public thunk::PPB_CompositorLayer_API {
 public:
  // Release callback for texture or image layer.
  typedef base::Callback<void(int32_t, const gpu::SyncToken&, bool)>
      ReleaseCallback;

  CompositorLayerResource(Connection connection,
                          PP_Instance instance,
                          const CompositorResource* compositor);

  const CompositorLayerData& data() const { return data_; }
  const ReleaseCallback& release_callback() const {
    return release_callback_;
  }
  void ResetReleaseCallback() { release_callback_.Reset(); }
  void Invalidate() { compositor_ = NULL; }

 private:
  enum LayerType {
    TYPE_COLOR,
    TYPE_TEXTURE,
    TYPE_IMAGE,
  };

  ~CompositorLayerResource() override;

  // Resource overrides:
  thunk::PPB_CompositorLayer_API* AsPPB_CompositorLayer_API() override;

  // thunk::PPB_Compositor_API overrides:
  int32_t SetColor(float red,
                   float green,
                   float blue,
                   float alpha,
                   const PP_Size* size) override;
  int32_t SetTexture0_1(
      PP_Resource context,
      uint32_t texture,
      const PP_Size* size,
      const scoped_refptr<ppapi::TrackedCallback>& callback) override;
  int32_t SetTexture(
      PP_Resource context,
      uint32_t target,
      uint32_t texture,
      const PP_Size* size,
      const scoped_refptr<TrackedCallback>& callback) override;
  int32_t SetImage(
      PP_Resource image_data,
      const PP_Size* size,
      const scoped_refptr<TrackedCallback>& callback) override;
  int32_t SetClipRect(const PP_Rect* rect) override;
  int32_t SetTransform(const float matrix[16]) override;
  int32_t SetOpacity(float opacity) override;
  int32_t SetBlendMode(PP_BlendMode mode) override;
  int32_t SetSourceRect(const PP_FloatRect* rect) override;
  int32_t SetPremultipliedAlpha(PP_Bool premult) override;

  bool SetType(LayerType type);
  int32_t CheckForSetTextureAndImage(
      LayerType type,
      const scoped_refptr<TrackedCallback>& release_callback);

  // The CompositorResource which own the layer. The layer is invalidated if
  // compositor_ is NULL.
  const CompositorResource* compositor_;

  // Release callback for uncommitted texture or image. When CommitLayers() on
  // the compositor_ is called, the callback will be copied into a map in the
  // compositor_, and it will be reset to NULL.
  ReleaseCallback release_callback_;

  // Size of texture or image. It is used to verify the rect arg of
  // SetSourceRect().
  PP_FloatSize source_size_;

  // Layer data.
  CompositorLayerData data_;

  DISALLOW_COPY_AND_ASSIGN(CompositorLayerResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_COMPOSITOR_LAYER_RESOURCE_H_
