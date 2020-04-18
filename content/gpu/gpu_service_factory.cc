// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/gpu/gpu_service_factory.h"

#include <memory>

#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/shape_detection/public/mojom/constants.mojom.h"
#include "services/shape_detection/shape_detection_service.h"

#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)
#include "base/bind.h"
#include "media/mojo/services/media_service_factory.h"  // nogncheck
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "content/public/gpu/content_gpu_client.h"
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)
#endif  // BUILDFLAG(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)

namespace content {

GpuServiceFactory::GpuServiceFactory(
    const gpu::GpuPreferences& gpu_preferences,
    const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
    base::WeakPtr<media::MediaGpuChannelManager> media_gpu_channel_manager,
    media::AndroidOverlayMojoFactoryCB android_overlay_factory_cb) {
#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)
  gpu_preferences_ = gpu_preferences;
  gpu_workarounds_ = gpu_workarounds;
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
  media_gpu_channel_manager_ = std::move(media_gpu_channel_manager);
  android_overlay_factory_cb_ = std::move(android_overlay_factory_cb);
#endif
}

GpuServiceFactory::~GpuServiceFactory() {}

void GpuServiceFactory::RegisterServices(ServiceMap* services) {
#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)
  media::CdmProxyFactoryCB cdm_proxy_factory_cb;
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  cdm_proxy_factory_cb =
      base::BindRepeating(&ContentGpuClient::CreateCdmProxy,
                          base::Unretained(GetContentClient()->gpu()));
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

  service_manager::EmbeddedServiceInfo info;
  info.factory = base::BindRepeating(
      &media::CreateGpuMediaService, gpu_preferences_, gpu_workarounds_,
      task_runner_, media_gpu_channel_manager_, android_overlay_factory_cb_,
      std::move(cdm_proxy_factory_cb));
  // This service will host audio/video decoders, and if these decoding
  // operations are blocked, user may hear audio glitch or see video freezing,
  // hence "user blocking".
  // TODO(crbug.com/786169): Check whether this needs to be single threaded.
  info.task_runner = base::CreateSingleThreadTaskRunnerWithTraits(
      {base::TaskPriority::USER_BLOCKING});
  services->insert(std::make_pair("media", info));
#endif  // BUILDFLAG(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)

  service_manager::EmbeddedServiceInfo shape_detection_info;
  shape_detection_info.factory =
      base::Bind(&shape_detection::ShapeDetectionService::Create);
  services->insert(std::make_pair(shape_detection::mojom::kServiceName,
                                  shape_detection_info));
}

}  // namespace content
