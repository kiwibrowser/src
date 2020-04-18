// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/mock_video_decoder_for_mixer.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/time/time.h"

namespace chromecast {
namespace media {

namespace {  // namespace

int64_t RoundDownToNearestMultiple(int64_t number, int64_t multiple) {
  return number - (number % multiple);
}

}  // namespace

template <int64_t CRN, int64_t CRD, int64_t CF>
MockVideoDecoderForMixer<CRN, CRD, CF>::MockVideoDecoderForMixer() {
  DCHECK(linear_clock_rate_ != 0.0);
}

template <int64_t CRN, int64_t CRD, int64_t CF>
void MockVideoDecoderForMixer<CRN, CRD, CF>::Initialize() {}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::Start(int64_t start_pts,
                                                   bool need_avsync) {
  VLOG(4) << "start_pts=" << start_pts;
  start_pts_ = start_pts;
  vsync_timer_.Start(FROM_HERE,
                     base::TimeDelta::FromMicroseconds(
                         std::round(GetVsyncPeriodUs() / linear_clock_rate_)),
                     this, &MockVideoDecoderForMixer::UpkeepVsync);
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
void MockVideoDecoderForMixer<CRN, CRD, CF>::Stop() {
  vsync_timer_.Stop();
  current_video_pts_ = INT64_MIN;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::Pause() {
  vsync_timer_.Stop();
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::Resume() {
  vsync_timer_.Start(FROM_HERE,
                     base::TimeDelta::FromMicroseconds(
                         std::round(GetVsyncPeriodUs() / linear_clock_rate_)),
                     this, &MockVideoDecoderForMixer::UpkeepVsync);
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::GetCurrentPts(int64_t* timestamp,
                                                           int64_t* pts) const {
  *timestamp = 0;
  *pts = last_displayed_frame_pts_;
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::SetPlaybackRate(float rate) {
  linear_pts_rate_ = rate;
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::SetPts(int64_t timestamp,
                                                    int64_t pts) {
  current_video_pts_ = pts;
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetDroppedFrames() {
  return dropped_frames_;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetRepeatedFrames() {
  return repeated_frames_;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetOutputRefreshRate() {
  return 60000;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetCurrentContentRefreshRate() {
  return content_fps_ * 1000;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
void MockVideoDecoderForMixer<CRN, CRD, CF>::SetDelegate(
    MediaPipelineBackend::Decoder::Delegate* delegate) {}

template <int64_t CRN, int64_t CRD, int64_t CF>
MediaPipelineBackend::BufferStatus
MockVideoDecoderForMixer<CRN, CRD, CF>::PushBuffer(CastDecoderBuffer* buffer) {
  return MediaPipelineBackend::kBufferSuccess;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
void MockVideoDecoderForMixer<CRN, CRD, CF>::GetStatistics(
    Statistics* statistics) {}

template <int64_t CRN, int64_t CRD, int64_t CF>
bool MockVideoDecoderForMixer<CRN, CRD, CF>::SetConfig(
    const VideoConfig& config) {
  return true;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetVsyncPeriodUs() {
  return std::round(1000000.0 / display_refresh_rate_);
}

template <int64_t CRN, int64_t CRD, int64_t CF>
void MockVideoDecoderForMixer<CRN, CRD, CF>::UpkeepVsync() {
  int64_t frame_period_us = std::round(1000000.0 / content_fps_);
  current_video_pts_ += std::round(GetVsyncPeriodUs() * linear_pts_rate_);

  if (current_video_pts_ > start_pts_) {
    int64_t current_displayed_frame =
        RoundDownToNearestMultiple(current_video_pts_, frame_period_us);

    int64_t difference_in_frames =
        (current_displayed_frame - last_displayed_frame_pts_) / frame_period_us;

    if (difference_in_frames != 1) {
      dropped_frames_ +=
          std::max(difference_in_frames - 1, static_cast<int64_t>(0));
      repeated_frames_ +=
          std::max(1 - difference_in_frames, static_cast<int64_t>(0));
      VLOG(4) << "last_displayed_frame_pts_=" << last_displayed_frame_pts_
              << " current_displayed_frame=" << current_displayed_frame
              << " difference_in_frames=" << difference_in_frames
              << " difference_in_time="
              << (current_displayed_frame - last_displayed_frame_pts_)
              << " dropped_frames_=" << dropped_frames_;
    }

    last_displayed_frame_pts_ = current_displayed_frame;
  }
}

template <int64_t CRN, int64_t CRD, int64_t CF>
MockVideoDecoderForMixer<CRN, CRD, CF>::~MockVideoDecoderForMixer() {}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetExpectedDroppedFrames() {
  int64_t expected_dropped_frames =
      std::round((1.0 - linear_clock_rate_) * content_fps_);
  return std::max(static_cast<int64_t>(0), expected_dropped_frames);
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetExpectedRepeatedFrames() {
  int64_t expected_repeated_frames =
      std::round((linear_clock_rate_ - 1.0) * content_fps_);
  return std::max(static_cast<int64_t>(0), expected_repeated_frames);
}

std::unique_ptr<VideoDecoderForMixer> VideoDecoderForMixer::Create(
    const MediaPipelineDeviceParams& params) {
  return MockVideoDecoderForMixer<1, 1, 60>::Create();
}

template <int64_t CRN, int64_t CRD, int64_t CF>
std::unique_ptr<VideoDecoderForTest>
MockVideoDecoderForMixer<CRN, CRD, CF>::Create() {
  return std::unique_ptr<VideoDecoderForTest>(
      new MockVideoDecoderForMixer<CRN, CRD, CF>());
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetNumberOfFramesTolerated() {
  if (CRN == 1 && CRD == 1) {
    // TODO(almasrymina): somehow the 30fps normal decoder very ocasionally
    // drops frames...?
    if (CF == 30)
      return 1;
    else
      return 0;
  }
  return 6;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t
MockVideoDecoderForMixer<CRN, CRD, CF>::GetNumberOfHardCorrectionsTolerated() {
  return 0;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t
MockVideoDecoderForMixer<CRN, CRD, CF>::GetNumberOfSoftCorrectionsTolerated() {
  return 1;
}

template <int64_t CRN, int64_t CRD, int64_t CF>
int64_t MockVideoDecoderForMixer<CRN, CRD, CF>::GetAvSyncDriftTolerated() {
  if (CRN == 1 && CRD == 1)
    return 25000;
  // TODO(almasrymina): really need to tighten this. The problem is that on
  // very broken decoders (130% speed) AV starts in sync, and then drifts while
  // we're building our vpts slope. Then we start correcting but we've drifted
  // so much that we need some time to get back in sync, which triggers our
  // unittests.
  return 100000;
}

template class MockVideoDecoderForMixer<1, 1, 30>;
template class MockVideoDecoderForMixer<1, 1, 24>;
template class MockVideoDecoderForMixer<1, 1, 60>;
template class MockVideoDecoderForMixer<2, 1, 60>;
template class MockVideoDecoderForMixer<1, 2, 60>;
template class MockVideoDecoderForMixer<14, 10, 60>;
template class MockVideoDecoderForMixer<13, 10, 60>;
template class MockVideoDecoderForMixer<12, 10, 60>;
template class MockVideoDecoderForMixer<11, 10, 60>;
template class MockVideoDecoderForMixer<9, 10, 60>;
template class MockVideoDecoderForMixer<8, 10, 60>;
template class MockVideoDecoderForMixer<7, 10, 60>;
template class MockVideoDecoderForMixer<6, 10, 60>;

}  // namespace media
}  // namespace chromecast
