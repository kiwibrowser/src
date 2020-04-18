// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/backend/desktop/media_pipeline_backend_desktop.h"
#include "chromecast/public/cast_media_shlib.h"
#include "chromecast/public/graphics_types.h"
#include "chromecast/public/media/media_capabilities_shlib.h"
#include "chromecast/public/media/media_pipeline_device_params.h"
#include "chromecast/public/video_plane.h"

namespace chromecast {
namespace media {
namespace {

class DesktopVideoPlane : public VideoPlane {
 public:
  ~DesktopVideoPlane() override {}

  void SetGeometry(const RectF& display_rect, Transform transform) override {}
};

DesktopVideoPlane* g_video_plane = nullptr;
base::ThreadTaskRunnerHandle* g_thread_task_runner_handle = nullptr;

}  // namespace

void CastMediaShlib::Initialize(const std::vector<std::string>& argv) {
  g_video_plane = new DesktopVideoPlane();
}

void CastMediaShlib::Finalize() {
  delete g_video_plane;
  g_video_plane = nullptr;
  delete g_thread_task_runner_handle;
  g_thread_task_runner_handle = nullptr;
}

VideoPlane* CastMediaShlib::GetVideoPlane() {
  return g_video_plane;
}

MediaPipelineBackend* CastMediaShlib::CreateMediaPipelineBackend(
    const MediaPipelineDeviceParams& params) {
  // Set up the static reference in base::ThreadTaskRunnerHandle::Get
  // for the media thread in this shared library.  We can extract the
  // SingleThreadTaskRunner passed in from cast_shell for this.
  if (!base::ThreadTaskRunnerHandle::IsSet()) {
    DCHECK(!g_thread_task_runner_handle);
    const scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        static_cast<TaskRunnerImpl*>(params.task_runner)->runner();
    DCHECK(task_runner->BelongsToCurrentThread());
    g_thread_task_runner_handle = new base::ThreadTaskRunnerHandle(task_runner);
  }

  return new MediaPipelineBackendDesktop();
}

double CastMediaShlib::GetMediaClockRate() {
  return 0.0;
}

double CastMediaShlib::MediaClockRatePrecision() {
  return 0.0;
}

void CastMediaShlib::MediaClockRateRange(double* minimum_rate,
                                         double* maximum_rate) {}

bool CastMediaShlib::SetMediaClockRate(double new_rate) {
  return false;
}

bool CastMediaShlib::SupportsMediaClockRateChange() {
  return false;
}

bool MediaCapabilitiesShlib::IsSupportedVideoConfig(VideoCodec codec,
                                                    VideoProfile profile,
                                                    int level) {
  return (codec == kCodecH264 || codec == kCodecVP8);
}

bool MediaCapabilitiesShlib::IsSupportedAudioConfig(const AudioConfig& config) {
  return config.codec == kCodecAAC || config.codec == kCodecMP3 ||
         config.codec == kCodecPCM || config.codec == kCodecVorbis;
}

}  // namespace media
}  // namespace chromecast
