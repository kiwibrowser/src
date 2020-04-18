// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_COMPOSITOR_HOST_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_COMPOSITOR_HOST_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/resources/shared_bitmap_id_registrar.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/shared_impl/compositor_layer_data.h"

namespace base {
class SharedMemory;
}  // namespace

namespace cc {
class CrossThreadSharedBitmap;
class Layer;
}  // namespace cc

namespace gpu {
struct SyncToken;
}  // namespace gpu

namespace content {

class PepperPluginInstanceImpl;
class RendererPpapiHost;

class PepperCompositorHost : public ppapi::host::ResourceHost {
 public:
  PepperCompositorHost(RendererPpapiHost* host,
                       PP_Instance instance,
                       PP_Resource resource);
  ~PepperCompositorHost() override;

  // Associates this device with the given plugin instance. You can pass NULL
  // to clear the existing device. Returns true on success. In this case, a
  // repaint of the page will also be scheduled. Failure means that the device
  // is already bound to a different instance, and nothing will happen.
  bool BindToInstance(PepperPluginInstanceImpl* new_instance);

  const scoped_refptr<cc::Layer> layer() { return layer_; };

  void ViewInitiatedPaint();

  void set_viewport_to_dip_scale(float viewport_to_dip_scale) {
    DCHECK_LT(0, viewport_to_dip_scale_);
    viewport_to_dip_scale_ = viewport_to_dip_scale;
  }

 private:
  void ImageReleased(int32_t id,
                     scoped_refptr<cc::CrossThreadSharedBitmap> shared_bitmap,
                     cc::SharedBitmapIdRegistration registration,
                     const gpu::SyncToken& sync_token,
                     bool is_lost);
  void ResourceReleased(int32_t id,
                        const gpu::SyncToken& sync_token,
                        bool is_lost);
  void SendCommitLayersReplyIfNecessary();
  void UpdateLayer(const scoped_refptr<cc::Layer>& layer,
                   const ppapi::CompositorLayerData* old_layer,
                   const ppapi::CompositorLayerData* new_layer,
                   std::unique_ptr<base::SharedMemory> image_shm);

  // ResourceMessageHandler overrides:
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

  // ppapi::host::ResourceHost overrides:
  bool IsCompositorHost() override;

  // Message handlers:
  int32_t OnHostMsgCommitLayers(
      ppapi::host::HostMessageContext* context,
      const std::vector<ppapi::CompositorLayerData>& layers,
      bool reset);

  // Non-owning pointer to the plugin instance this context is currently bound
  // to, if any. If the context is currently unbound, this will be NULL.
  PepperPluginInstanceImpl* bound_instance_;

  // The toplevel cc::Layer. It is the parent of other cc::Layers.
  scoped_refptr<cc::Layer> layer_;

  // A list of layers. It is used for updating layers' properties in
  // subsequent CommitLayers() calls.
  struct LayerData {
    LayerData(const scoped_refptr<cc::Layer>& cc,
              const ppapi::CompositorLayerData& pp);
    LayerData(const LayerData& other);
    ~LayerData();

    scoped_refptr<cc::Layer> cc_layer;
    ppapi::CompositorLayerData pp_layer;
  };
  std::vector<LayerData> layers_;

  ppapi::host::ReplyMessageContext commit_layers_reply_context_;

  // The scale between the viewport and dip. This differs in
  // use-zoom-for-dsf mode where the content is scaled by zooming.
  float viewport_to_dip_scale_ = 1.0f;

  base::WeakPtrFactory<PepperCompositorHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperCompositorHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_COMPOSITOR_HOST_H_
