// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_AV_SYNC_VIDEO_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_AV_SYNC_VIDEO_H_

#include <cstdint>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/timer/timer.h"
#include "chromecast/base/statistics/weighted_moving_linear_regression.h"
#include "chromecast/media/cma/backend/av_sync.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace chromecast {
namespace media {
class MediaPipelineBackendForMixer;

class AvSyncVideo : public AvSync {
 public:
  AvSyncVideo(const scoped_refptr<base::SingleThreadTaskRunner> task_runner,
              MediaPipelineBackendForMixer* const backend);

  ~AvSyncVideo() override;

  // AvSync implementation:
  void NotifyStart(int64_t timestamp) override;
  void NotifyStop() override;
  void NotifyPause() override;
  void NotifyResume() override;

  class Delegate {
   public:
    virtual void NotifyAvSyncPlaybackStatistics(
        int64_t unexpected_dropped_frames,
        int64_t unexpected_repeated_frames,
        double average_av_sync_difference,
        int64_t current_apts_us,
        int64_t current_vpts_us,
        int64_t number_of_soft_corrections,
        int64_t number_of_hard_corrections) = 0;

    virtual ~Delegate() = default;
  };

  void SetDelegate(Delegate* delegate) {
    DCHECK(delegate);
    delegate_ = delegate;
  }

 private:
  void UpkeepAvSync();
  void StartAvSync();
  void StopAvSync();
  void GatherPlaybackStatistics();

  void SoftCorrection(int64_t now);
  void InSyncCorrection(int64_t now);

  Delegate* delegate_ = nullptr;

  int64_t av_sync_difference_sum_ = 0;
  int64_t av_sync_difference_count_ = 0;

  base::RepeatingTimer upkeep_av_sync_timer_;
  base::RepeatingTimer playback_statistics_timer_;
  bool in_soft_correction_ = false;
  int64_t difference_at_start_of_correction_ = 0;

  // TODO(almasrymina): having a linear regression for the audio pts is
  // dangerous, because glitches in the audio or intentional changes in the
  // playback rate will propagate to the regression at a delay. Consider
  // reducing the lifetime of data or firing an event to the av sync module
  // that will reset the linear regression model.
  std::unique_ptr<WeightedMovingLinearRegression> audio_pts_;
  std::unique_ptr<WeightedMovingLinearRegression> video_pts_;
  std::unique_ptr<WeightedMovingLinearRegression> error_;
  double current_audio_playback_rate_ = 1.0;

  int64_t last_gather_timestamp_us_ = 0;
  int64_t last_repeated_frames_ = 0;
  int64_t last_dropped_frames_ = 0;
  int64_t number_of_hard_corrections_ = 0;
  int64_t number_of_soft_corrections_ = 0;
  int64_t last_vpts_value_recorded_ = 0;
  int64_t last_correction_timestamp_us = INT64_MIN;
  int64_t av_sync_start_timestamp_ = INT64_MIN;

  MediaPipelineBackendForMixer* const backend_;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_AV_SYNC_VIDEO_H_
