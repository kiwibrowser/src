// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_SERVICE_CAST_MOJO_MEDIA_CLIENT_H_
#define CHROMECAST_MEDIA_SERVICE_CAST_MOJO_MEDIA_CLIENT_H_

#include <memory>
#include <string>

#include "media/mojo/services/mojo_media_client.h"

namespace chromecast {
namespace media {

class CmaBackendFactory;
class MediaResourceTracker;
class VideoModeSwitcher;
class VideoResolutionPolicy;

class CastMojoMediaClient : public ::media::MojoMediaClient {
 public:
  using CreateCdmFactoryCB =
      base::Callback<std::unique_ptr<::media::CdmFactory>()>;

  CastMojoMediaClient(CmaBackendFactory* backend_factory,
                      const CreateCdmFactoryCB& create_cdm_factory_cb,
                      VideoModeSwitcher* video_mode_switcher,
                      VideoResolutionPolicy* video_resolution_policy,
                      MediaResourceTracker* media_resource_tracker);
  ~CastMojoMediaClient() override;

  // MojoMediaClient overrides.
  void Initialize(service_manager::Connector* connector) override;
  std::unique_ptr<::media::Renderer> CreateRenderer(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      ::media::MediaLog* media_log,
      const std::string& audio_device_id) override;
  std::unique_ptr<::media::CdmFactory> CreateCdmFactory(
      service_manager::mojom::InterfaceProvider* host_interfaces) override;

 private:
  service_manager::Connector* connector_;
  CmaBackendFactory* const backend_factory_;
  const CreateCdmFactoryCB create_cdm_factory_cb_;
  VideoModeSwitcher* video_mode_switcher_;
  VideoResolutionPolicy* video_resolution_policy_;
  MediaResourceTracker* media_resource_tracker_;

  DISALLOW_COPY_AND_ASSIGN(CastMojoMediaClient);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_SERVICE_CAST_MOJO_MEDIA_CLIENT_H_
