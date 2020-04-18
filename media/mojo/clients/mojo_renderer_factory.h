// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_RENDERER_FACTORY_H_
#define MEDIA_MOJO_CLIENTS_MOJO_RENDERER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "media/base/renderer_factory.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "media/mojo/interfaces/renderer.mojom.h"

namespace service_manager {
class InterfaceProvider;
}

namespace media {

class GpuVideoAcceleratorFactories;

// The default factory class for creating MojoRenderer.
class MojoRendererFactory : public RendererFactory {
 public:
  using GetGpuFactoriesCB = base::Callback<GpuVideoAcceleratorFactories*()>;
  using GetTypeSpecificIdCB = base::Callback<std::string()>;

  MojoRendererFactory(mojom::HostedRendererType type,
                      const GetGpuFactoriesCB& get_gpu_factories_cb,
                      media::mojom::InterfaceFactory* interface_factory);

  ~MojoRendererFactory() final;

  std::unique_ptr<Renderer> CreateRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      AudioRendererSink* audio_renderer_sink,
      VideoRendererSink* video_renderer_sink,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space) final;

  // Sets the callback that will fetch the TypeSpecificId when
  // InterfaceFactory::CreateRenderer() is called. What the string represents
  // depends on the value of |hosted_renderer_type_|. Currently, we only use it
  // with mojom::HostedRendererType::kFlinging, in which case
  // |get_type_specific_id| should return the presentation ID to be given to the
  // FlingingRenderer in the browser process.
  void SetGetTypeSpecificIdCB(const GetTypeSpecificIdCB& get_type_specific_id);

 private:
  mojom::RendererPtr GetRendererPtr();

  GetGpuFactoriesCB get_gpu_factories_cb_;
  GetTypeSpecificIdCB get_type_specific_id_;

  // InterfaceFactory or InterfaceProvider used to create or connect to remote
  // renderer.
  media::mojom::InterfaceFactory* interface_factory_ = nullptr;

  // Underlying renderer type that will be hosted by the MojoRenderer.
  mojom::HostedRendererType hosted_renderer_type_;

  DISALLOW_COPY_AND_ASSIGN(MojoRendererFactory);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_RENDERER_FACTORY_H_
