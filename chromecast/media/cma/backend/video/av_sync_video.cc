// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/video/av_sync_video.h"

#include <algorithm>
#include <iomanip>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "chromecast/media/cma/backend/audio_decoder_for_mixer.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_for_mixer.h"
#include "chromecast/media/cma/backend/video_decoder_for_mixer.h"

namespace chromecast {
namespace media {

namespace {
const int64_t kInvalidTimestamp = std::numeric_limits<int64_t>::min();

// Threshold where the audio and video pts are far enough apart such that we
// want to do a small correction.
const int kSoftCorrectionThresholdUs = 16000;

// When doing a soft correction, we will do so by changing the rate of video
// playback. These constants define the multiplier in either direction.
const double kRateReduceMultiplier = 0.99;
const double kRateIncreaseMultiplier = 1.01;

// Length of time after which data is forgotten from our linear regression
// models.
const int kLinearRegressionDataLifetimeUs = 5000000;

// Time interval between AV sync upkeeps.
constexpr base::TimeDelta kAvSyncUpkeepInterval =
    base::TimeDelta::FromMilliseconds(10);

#if DCHECK_IS_ON()
// Time interval between checking playbacks statistics.
constexpr base::TimeDelta kPlaybackStatisticsCheckInterval =
    base::TimeDelta::FromSeconds(1);
#endif

// The amount of time we wait after a correction before we start upkeeping the
// AV sync.
const int kMinimumWaitAfterCorrectionUs = 200000;
}  // namespace

std::unique_ptr<AvSync> AvSync::Create(
    const scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    MediaPipelineBackendForMixer* const backend) {
  return std::make_unique<AvSyncVideo>(task_runner, backend);
}

AvSyncVideo::AvSyncVideo(
    const scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    MediaPipelineBackendForMixer* const backend)
    : audio_pts_(
          new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs)),
      video_pts_(
          new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs)),
      error_(
          new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs)),
      backend_(backend) {
  DCHECK(backend_);
}

void AvSyncVideo::UpkeepAvSync() {
  if (backend_->MonotonicClockNow() < av_sync_start_timestamp_) {
    return;
  }

  if (!backend_->video_decoder() || !backend_->audio_decoder()) {
    VLOG(4) << "No video decoder available.";
    return;
  }

  // Currently the audio pipeline doesn't seem to return valid values for the
  // PTS after changing the playback rate.
  if (last_correction_timestamp_us != INT64_MIN &&
      backend_->MonotonicClockNow() - last_correction_timestamp_us <
          kMinimumWaitAfterCorrectionUs) {
    return;
  }

  int64_t now = backend_->MonotonicClockNow();
  int64_t current_apts = 0;
  double error = 0.0;

  int64_t new_current_vpts = 0;
  int64_t timestamp = 0;
  backend_->video_decoder()->GetCurrentPts(&timestamp, &new_current_vpts);
  if (new_current_vpts != last_vpts_value_recorded_) {
    video_pts_->AddSample(now, new_current_vpts, 1.0);
    last_vpts_value_recorded_ = new_current_vpts;
  }

  // TODO(almasrymina): using GetCurrentPts is vulnerable to pairing it with an
  // outdated 'now' if the thread gets descheduled in between. We currently see
  // extraneous corrections on real hardware and it's probably due to this.
  //
  // Consider either going back to a NotifyAudioBufferPushed approach that
  // works, or improving GetCurrentPts such as it returns the timestamp this
  // was last updated at.
  // TODO(almasrymina): b/78592779. AudioDecoderForMixer::GetCurrentPts seems
  // to return invalid values which are not kInvalidTimestamp, for unknown
  // reasons. As a workaround until that issue is root caused, ignore
  // GetCurrentPts values that are way off.
  int64_t audio_pts = backend_->audio_decoder()->GetCurrentPts();
  if (abs(audio_pts - new_current_vpts) >
          (1ll * 60ll * 60ll * 1000000ll) /* 1hr in us */
      || audio_pts == kInvalidTimestamp) {
    LOG(WARNING) << "Audio decoder returned invalid pts=" << audio_pts
                 << " new_current_vpts=" << new_current_vpts;
  } else {
    audio_pts_->AddSample(now, backend_->audio_decoder()->GetCurrentPts(), 1.0);
  }

  if (video_pts_->num_samples() < 10 || audio_pts_->num_samples() < 20) {
    VLOG(4) << "Linear regression samples too little."
            << " video_pts_->num_samples()=" << video_pts_->num_samples()
            << " audio_pts_->num_samples()=" << audio_pts_->num_samples();
    return;
  }

  int64_t current_vpts;
  double vpts_slope;
  double apts_slope;
  video_pts_->EstimateY(now, &current_vpts, &error);
  audio_pts_->EstimateY(now, &current_apts, &error);
  video_pts_->EstimateSlope(&vpts_slope, &error);
  audio_pts_->EstimateSlope(&apts_slope, &error);

  error_->AddSample(now, current_apts - current_vpts, 1.0);

  if (error_->num_samples() < 5) {
    VLOG(4)
        << "Error linear regression samples too little. error_->num_samples()="
        << error_->num_samples() << " vpts_slope=" << vpts_slope;
    return;
  }

  int64_t difference;
  error_->EstimateY(now, &difference, &error);

  VLOG(3) << "Pts_monitor."
          << " difference=" << difference / 1000 << " apts_slope=" << apts_slope
          << " apts_slope=" << apts_slope
          << " current_audio_playback_rate_=" << current_audio_playback_rate_
          << " current_vpts=" << new_current_vpts
          << " current_apts=" << backend_->audio_decoder()->GetCurrentPts();

  av_sync_difference_sum_ += difference;
  ++av_sync_difference_count_;

  if (abs(difference) > kSoftCorrectionThresholdUs) {
    SoftCorrection(now);
  } else {
    InSyncCorrection(now);
  }
}

void AvSyncVideo::SoftCorrection(int64_t now) {
  int64_t current_apts = 0;
  int64_t current_vpts = 0;
  int64_t difference = 0;
  double error = 0.0;
  double apts_slope = 0.0;
  double vpts_slope = 0.0;

  video_pts_->EstimateY(now, &current_vpts, &error);
  audio_pts_->EstimateY(now, &current_apts, &error);
  video_pts_->EstimateSlope(&vpts_slope, &error);
  audio_pts_->EstimateSlope(&apts_slope, &error);
  error_->EstimateY(now, &difference, &error);

  if (audio_pts_->num_samples() < 50) {
    VLOG(4) << "Not enough apts samples=" << video_pts_->num_samples();
    return;
  }

  if (in_soft_correction_ &&
      std::abs(difference) < difference_at_start_of_correction_) {
    VLOG(4) << " difference=" << difference / 1000
            << " difference_at_start_of_correction_="
            << difference_at_start_of_correction_ / 1000;
    return;
  }

  double factor = current_apts > current_vpts ? kRateReduceMultiplier
                                              : kRateIncreaseMultiplier;
  current_audio_playback_rate_ *= (vpts_slope * factor / apts_slope);

  current_audio_playback_rate_ =
      backend_->audio_decoder()->SetPlaybackRate(current_audio_playback_rate_);

  number_of_soft_corrections_++;
  in_soft_correction_ = true;
  difference_at_start_of_correction_ = abs(difference);

  audio_pts_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));
  error_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));

  LOG(INFO) << "Soft Correction."
            << " difference=" << difference / 1000
            << " apts_slope=" << apts_slope << " vpts_slope=" << vpts_slope
            << " current_apts=" << current_apts
            << " current_vpts=" << current_vpts
            << " current_audio_playback_rate_=" << current_audio_playback_rate_;

  last_correction_timestamp_us = backend_->MonotonicClockNow();
}

// This method only does anything if in_soft_correction_ == true, which is the
// case if the last correction we've executed is a soft_correction.
//
// The soft correction will aim to bridge the gap between the audio and video,
// and so after the soft correction is executed, the audio and video rate of
// playback will not be equal.
//
// This 'correction' gets executed when the audio and video PTS are
// sufficiently close to each other, and we no longer need to bridge a gap
// between them. This method will have it so that vpts_slope == apts_slope, and
// the content should continue to play in sync from here on out.
void AvSyncVideo::InSyncCorrection(int64_t now) {
  if (audio_pts_->num_samples() < 50 || !in_soft_correction_) {
    return;
  }

  int64_t current_apts = 0;
  int64_t current_vpts = 0;
  int64_t difference = 0;
  double error = 0.0;
  double apts_slope = 0.0;
  double vpts_slope = 0.0;

  video_pts_->EstimateY(now, &current_vpts, &error);
  audio_pts_->EstimateY(now, &current_apts, &error);
  video_pts_->EstimateSlope(&vpts_slope, &error);
  audio_pts_->EstimateSlope(&apts_slope, &error);

  current_audio_playback_rate_ *= vpts_slope / apts_slope;
  current_audio_playback_rate_ =
      backend_->audio_decoder()->SetPlaybackRate(current_audio_playback_rate_);
  in_soft_correction_ = false;
  difference_at_start_of_correction_ = 0;

  audio_pts_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));
  error_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));

  LOG(INFO) << "In sync Correction."
            << " difference=" << difference / 1000
            << " apts_slope=" << apts_slope << " vpts_slope=" << vpts_slope
            << " current_apts=" << current_apts
            << " current_vpts=" << current_vpts
            << " current_audio_playback_rate_=" << current_audio_playback_rate_;

  last_correction_timestamp_us = backend_->MonotonicClockNow();
}

void AvSyncVideo::GatherPlaybackStatistics() {
  DCHECK(backend_);
  if (!backend_->video_decoder()) {
    return;
  }

  int64_t frame_rate_difference =
      (backend_->video_decoder()->GetCurrentContentRefreshRate() -
       backend_->video_decoder()->GetOutputRefreshRate()) /
      1000;

  int64_t expected_dropped_frames_per_second =
      std::max<int64_t>(frame_rate_difference, 0);

  int64_t expected_repeated_frames_per_second =
      std::max<int64_t>(-frame_rate_difference, 0);

  int64_t current_time = backend_->MonotonicClockNow();

  int64_t expected_dropped_frames =
      std::round(expected_dropped_frames_per_second *
                 ((current_time - last_gather_timestamp_us_) / 1000000));

  int64_t expected_repeated_frames =
      std::round(expected_repeated_frames_per_second *
                 ((current_time - last_gather_timestamp_us_) / 1000000));

  int64_t dropped_frames = backend_->video_decoder()->GetDroppedFrames();
  int64_t repeated_frames = backend_->video_decoder()->GetRepeatedFrames();

  int64_t unexpected_dropped_frames =
      (dropped_frames - last_dropped_frames_) - expected_dropped_frames;
  int64_t unexpected_repeated_frames =
      (repeated_frames - last_repeated_frames_) - expected_repeated_frames;

  double average_av_sync_difference = 0.0;

  if (av_sync_difference_count_ != 0) {
    average_av_sync_difference = static_cast<double>(av_sync_difference_sum_) /
                                 static_cast<double>(av_sync_difference_count_);
  }
  av_sync_difference_sum_ = 0;
  av_sync_difference_count_ = 0;

  LOG(INFO) << "Playback diagnostics:"
            << " CurrentContentRefreshRate="
            << backend_->video_decoder()->GetCurrentContentRefreshRate()
            << " OutputRefreshRate="
            << backend_->video_decoder()->GetOutputRefreshRate()
            << " unexpected_dropped_frames=" << unexpected_dropped_frames
            << " unexpected_repeated_frames=" << unexpected_repeated_frames
            << " average_av_sync_difference="
            << average_av_sync_difference / 1000;

  int64_t current_vpts = 0;
  int64_t current_apts = 0;
  double error = 0.0;
  video_pts_->EstimateY(current_time, &current_vpts, &error);
  audio_pts_->EstimateY(current_time, &current_apts, &error);

  if (delegate_) {
    delegate_->NotifyAvSyncPlaybackStatistics(
        unexpected_dropped_frames, unexpected_repeated_frames,
        average_av_sync_difference, current_vpts, current_vpts,
        number_of_soft_corrections_, number_of_hard_corrections_);
  }

  last_gather_timestamp_us_ = current_time;
  last_repeated_frames_ = repeated_frames;
  last_dropped_frames_ = dropped_frames;
  number_of_soft_corrections_ = 0;
  number_of_hard_corrections_ = 0;
}

void AvSyncVideo::StopAvSync() {
  audio_pts_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));
  video_pts_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));
  error_.reset(
      new WeightedMovingLinearRegression(kLinearRegressionDataLifetimeUs));
  upkeep_av_sync_timer_.Stop();
  playback_statistics_timer_.Stop();
}

void AvSyncVideo::NotifyStart(int64_t timestamp) {
  number_of_soft_corrections_ = 0;
  number_of_hard_corrections_ = 0;
  in_soft_correction_ = false;
  difference_at_start_of_correction_ = 0;
  av_sync_start_timestamp_ = timestamp;

  StartAvSync();
}

void AvSyncVideo::NotifyStop() {
  StopAvSync();
  av_sync_start_timestamp_ = INT64_MIN;
}

void AvSyncVideo::NotifyPause() {
  StopAvSync();
}

void AvSyncVideo::NotifyResume() {
  StartAvSync();
}

void AvSyncVideo::StartAvSync() {
  upkeep_av_sync_timer_.Start(FROM_HERE, kAvSyncUpkeepInterval, this,
                              &AvSyncVideo::UpkeepAvSync);
#if DCHECK_IS_ON()
  // TODO(almasrymina): if this logic turns out to be useful for metrics
  // recording, keep it and remove this TODO. Otherwise remove it.
  playback_statistics_timer_.Start(FROM_HERE, kPlaybackStatisticsCheckInterval,
                                   this,
                                   &AvSyncVideo::GatherPlaybackStatistics);
#endif
}

AvSyncVideo::~AvSyncVideo() = default;

}  // namespace media
}  // namespace chromecast
