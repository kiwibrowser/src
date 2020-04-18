// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_SERVICE_CAST_RENDERER_H_
#define CHROMECAST_MEDIA_SERVICE_CAST_RENDERER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chromecast/media/base/media_resource_tracker.h"
#include "chromecast/media/base/video_resolution_policy.h"
#include "chromecast/media/cma/backend/cma_backend_factory.h"
#include "media/base/renderer.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace chromecast {
class TaskRunnerImpl;

namespace media {
class BalancedMediaTaskRunnerFactory;
class CastCdmContext;
class MediaPipelineImpl;
class VideoModeSwitcher;

class CastRenderer : public ::media::Renderer,
                     public VideoResolutionPolicy::Observer {
 public:
  CastRenderer(CmaBackendFactory* backend_factory,
               const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
               const std::string& audio_device_id,
               VideoModeSwitcher* video_mode_switcher,
               VideoResolutionPolicy* video_resolution_policy,
               MediaResourceTracker* media_resource_tracker);
  ~CastRenderer() final;

  // ::media::Renderer implementation.
  void Initialize(::media::MediaResource* media_resource,
                  ::media::RendererClient* client,
                  const ::media::PipelineStatusCB& init_cb) final;
  void SetCdm(::media::CdmContext* cdm_context,
              const ::media::CdmAttachedCB& cdm_attached_cb) final;
  void Flush(const base::Closure& flush_cb) final;
  void StartPlayingFrom(base::TimeDelta time) final;
  void SetPlaybackRate(double playback_rate) final;
  void SetVolume(float volume) final;
  base::TimeDelta GetMediaTime() final;

  // VideoResolutionPolicy::Observer implementation.
  void OnVideoResolutionPolicyChanged() override;

 private:
  enum Stream { STREAM_AUDIO, STREAM_VIDEO };
  void OnError(::media::PipelineStatus status);
  void OnEnded(Stream stream);
  void OnStatisticsUpdate(const ::media::PipelineStatistics& stats);
  void OnBufferingStateChange(::media::BufferingState state);
  void OnWaitingForDecryptionKey();
  void OnVideoNaturalSizeChange(const gfx::Size& size);
  void OnVideoOpacityChange(bool opaque);
  void CheckVideoResolutionPolicy();

  void OnVideoInitializationFinished(const ::media::PipelineStatusCB& init_cb,
                                     ::media::PipelineStatus status);

  CmaBackendFactory* const backend_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  const std::string audio_device_id_;
  VideoModeSwitcher* video_mode_switcher_;
  VideoResolutionPolicy* video_resolution_policy_;
  MediaResourceTracker* media_resource_tracker_;
  // Must outlive |pipeline_| to properly count resource usage.
  std::unique_ptr<MediaResourceTracker::ScopedUsage> media_resource_usage_;

  ::media::RendererClient* client_;
  CastCdmContext* cast_cdm_context_;
  scoped_refptr<BalancedMediaTaskRunnerFactory> media_task_runner_factory_;
  std::unique_ptr<TaskRunnerImpl> backend_task_runner_;
  std::unique_ptr<MediaPipelineImpl> pipeline_;
  bool eos_[2];
  gfx::Size video_res_;

  base::WeakPtrFactory<CastRenderer> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(CastRenderer);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_SERVICE_CAST_RENDERER_H_
