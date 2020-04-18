// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/service/cast_mojo_media_client.h"

#include "chromecast/media/cma/backend/cma_backend_factory.h"
#include "chromecast/media/service/cast_renderer.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "media/base/cdm_factory.h"
#include "media/base/media_log.h"
#include "media/base/overlay_info.h"

namespace chromecast {
namespace media {

CastMojoMediaClient::CastMojoMediaClient(
    CmaBackendFactory* backend_factory,
    const CreateCdmFactoryCB& create_cdm_factory_cb,
    VideoModeSwitcher* video_mode_switcher,
    VideoResolutionPolicy* video_resolution_policy,
    MediaResourceTracker* media_resource_tracker)
    : connector_(nullptr),
      backend_factory_(backend_factory),
      create_cdm_factory_cb_(create_cdm_factory_cb),
      video_mode_switcher_(video_mode_switcher),
      video_resolution_policy_(video_resolution_policy),
      media_resource_tracker_(media_resource_tracker) {
  DCHECK(backend_factory_);
}

CastMojoMediaClient::~CastMojoMediaClient() {}

void CastMojoMediaClient::Initialize(service_manager::Connector* connector) {
  DCHECK(!connector_);
  DCHECK(connector);
  connector_ = connector;
}

std::unique_ptr<::media::Renderer> CastMojoMediaClient::CreateRenderer(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    ::media::MediaLog* /* media_log */,
    const std::string& audio_device_id) {
  return std::make_unique<CastRenderer>(
      backend_factory_, task_runner, audio_device_id, video_mode_switcher_,
      video_resolution_policy_, media_resource_tracker_);
}

std::unique_ptr<::media::CdmFactory> CastMojoMediaClient::CreateCdmFactory(
    service_manager::mojom::InterfaceProvider* /* host_interfaces */) {
  return create_cdm_factory_cb_.Run();
}

}  // namespace media
}  // namespace chromecast
