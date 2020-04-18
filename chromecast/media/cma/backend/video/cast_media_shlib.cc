// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/public/cast_media_shlib.h"

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/backend/stream_mixer.h"
#include "chromecast/public/graphics_types.h"
#include "chromecast/public/media/media_capabilities_shlib.h"

#include "chromecast/media/cma/backend/media_pipeline_backend_for_mixer.h"

namespace chromecast {
namespace media {

base::AtExitManager g_at_exit_manager;

std::unique_ptr<base::ThreadTaskRunnerHandle> g_thread_task_runner_handle;

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
    g_thread_task_runner_handle.reset(
        new base::ThreadTaskRunnerHandle(task_runner));
  }

  return new MediaPipelineBackendForMixer(params);
}

void CastMediaShlib::Finalize() {
  g_thread_task_runner_handle.reset();
}
double CastMediaShlib::GetMediaClockRate() {
  return 0.0;
}

double CastMediaShlib::MediaClockRatePrecision() {
  return 0.0;
}

void CastMediaShlib::MediaClockRateRange(double* minimum_rate,
                                         double* maximum_rate) {
  *minimum_rate = 0.0;
  *maximum_rate = 1.0;
}

bool CastMediaShlib::SetMediaClockRate(double new_rate) {
  return false;
}

bool CastMediaShlib::SupportsMediaClockRateChange() {
  return false;
}

bool MediaCapabilitiesShlib::IsSupportedAudioConfig(const AudioConfig& config) {
  switch (config.codec) {
    case kCodecPCM:
    case kCodecPCM_S16BE:
    case kCodecAAC:
    case kCodecMP3:
    case kCodecVorbis:
      return true;
    default:
      break;
  }
  return false;
}

void CastMediaShlib::AddLoopbackAudioObserver(LoopbackAudioObserver* observer) {
  StreamMixer::Get()->AddLoopbackAudioObserver(observer);
}

void CastMediaShlib::RemoveLoopbackAudioObserver(
    LoopbackAudioObserver* observer) {
  StreamMixer::Get()->RemoveLoopbackAudioObserver(observer);
}

}  // namespace media
}  // namespace chromecast
