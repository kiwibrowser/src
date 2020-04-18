// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/public/cast_media_shlib.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/base/init_command_line_shlib.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_for_mixer.h"
#include "chromecast/media/cma/backend/stream_mixer.h"
#include "chromecast/public/graphics_types.h"
#include "chromecast/public/media/media_pipeline_device_params.h"
#include "chromecast/public/video_plane.h"
#include "media/base/media.h"
#include "media/base/media_switches.h"

namespace chromecast {
namespace media {

namespace {

// 1 MHz reference allows easy translation between frequency and PPM.
const double kMediaClockFrequency = 1e6;

class DefaultVideoPlane : public VideoPlane {
 public:
  ~DefaultVideoPlane() override {}

  void SetGeometry(const RectF& display_rect, Transform transform) override {}
};

DefaultVideoPlane* g_video_plane = nullptr;
base::ThreadTaskRunnerHandle* g_thread_task_runner_handle = nullptr;

}  // namespace

void CastMediaShlib::Initialize(const std::vector<std::string>& argv) {
  // On Fuchsia CastMediaShlib is compiled statically with cast_shell, so |argv|
  // can be ignored.

  g_video_plane = new DefaultVideoPlane();

  ::media::InitializeMediaLibrary();
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
  // Set up the static reference in base::ThreadTaskRunnerHandle::Get()
  // for the media thread in this shared library. We can extract the
  // SingleThreadTaskRunner passed in from cast_shell for this.
  if (!base::ThreadTaskRunnerHandle::IsSet()) {
    DCHECK(!g_thread_task_runner_handle);
    const scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        static_cast<TaskRunnerImpl*>(params.task_runner)->runner();

    DCHECK(task_runner->BelongsToCurrentThread());
    g_thread_task_runner_handle = new base::ThreadTaskRunnerHandle(task_runner);
  }

  return new MediaPipelineBackendForMixer(params);
}

double CastMediaShlib::GetMediaClockRate() {
  return kMediaClockFrequency;
}

double CastMediaShlib::MediaClockRatePrecision() {
  return 1.0;
}

void CastMediaShlib::MediaClockRateRange(double* minimum_rate,
                                         double* maximum_rate) {
  *minimum_rate = kMediaClockFrequency;
  *maximum_rate = kMediaClockFrequency;
}

bool CastMediaShlib::SetMediaClockRate(double new_rate) {
  NOTIMPLEMENTED();
  return new_rate == kMediaClockFrequency;
}

bool CastMediaShlib::SupportsMediaClockRateChange() {
  return false;
}

void CastMediaShlib::AddLoopbackAudioObserver(LoopbackAudioObserver* observer) {
  StreamMixer::Get()->AddLoopbackAudioObserver(observer);
}

void CastMediaShlib::RemoveLoopbackAudioObserver(
    LoopbackAudioObserver* observer) {
  StreamMixer::Get()->RemoveLoopbackAudioObserver(observer);
}

void CastMediaShlib::SetPostProcessorConfig(const std::string& name,
                                            const std::string& config) {
  StreamMixer::Get()->SetPostProcessorConfig(name, config);
}

}  // namespace media
}  // namespace chromecast
