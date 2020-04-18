// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AV_SYNC_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AV_SYNC_H_

#include <stdint.h>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace chromecast {
namespace media {

class MediaPipelineBackendForMixer;

// Interface to an AV sync module. This AV sync treats the video as master and
// syncs the audio to it.
//
// Whatever the owner of this component is, it should include and depend on
// this interface rather the implementation header file. It should be possible
// for someone in the future to provide their own implementation of this class
// by linking in their AvSync::Create method statically defined below.
class AvSync {
 public:
  static std::unique_ptr<AvSync> Create(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      MediaPipelineBackendForMixer* backend);

  virtual ~AvSync() = default;

  // Notify that the audio and video playback will start at |timestamp|.
  // |timestamp| is an absolute timestamp on CLOCK_MONOTONIC or
  // CLOCK_MONOTONIC_RAW. AvSync will typically start upkeeping AV sync after
  // this is called.
  // The AV sync code is *not* responsible for starting the video.
  virtual void NotifyStart(int64_t timestamp) = 0;

  // Notify that the audio playback has been stopped. AvSync will typically stop
  // upkeeping AV sync after this call. The AV sync code is *not* responsible
  // for stopping the video.
  virtual void NotifyStop() = 0;

  // Notify that the audio playback has been paused. AvSync will typically stop
  // upkeeping AV sync until the audio playback is resumed again. The AV sync
  // code is *not* responsible for pausing the video.
  virtual void NotifyPause() = 0;

  // Notify that the audio playback has been resumed. AvSync will typically
  // start upkeeping AV sync again after this is called. The AV sync code is
  // *not* responsible for resuming the video.
  virtual void NotifyResume() = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AV_SYNC_H_
