/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_SCENARIO_QUALITY_STATS_H_
#define TEST_SCENARIO_QUALITY_STATS_H_

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "api/units/timestamp.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "rtc_base/task_queue.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/clock.h"
#include "test/logging/log_writer.h"
#include "test/scenario/quality_info.h"
#include "test/scenario/scenario_config.h"
#include "test/statistics.h"

namespace webrtc {
namespace test {

class VideoQualityAnalyzer {
 public:
  VideoQualityAnalyzer(
      std::unique_ptr<RtcEventLogOutput> writer,
      std::function<void(const VideoFrameQualityInfo&)> frame_info_handler);
  ~VideoQualityAnalyzer();
  void OnCapturedFrame(const VideoFrame& frame);
  void OnDecodedFrame(const VideoFrame& frame);
  void Synchronize();
  bool Active() const;
  Clock* clock();

 private:
  int64_t DecodedFrameCaptureTimeOffsetMs(const VideoFrame& decoded) const;
  int64_t CapturedFrameCaptureTimeOffsetMs(const VideoFrame& captured) const;
  void PrintHeaders();
  void PrintFrameInfo(const VideoFrameQualityInfo& sample);
  const std::unique_ptr<RtcEventLogOutput> writer_;
  std::vector<std::function<void(const VideoFrameQualityInfo&)>>
      frame_info_handlers_;
  std::deque<VideoFrame> captured_frames_;
  absl::optional<int64_t> first_capture_ntp_time_ms_;
  absl::optional<uint32_t> first_decode_rtp_timestamp_;
  rtc::TaskQueue task_queue_;
};

struct VideoQualityStats {
  int total = 0;
  int valid = 0;
  int lost = 0;
  Statistics end_to_end_seconds;
  Statistics frame_size;
  Statistics psnr;
  Statistics ssim;

  void HandleFrameInfo(VideoFrameQualityInfo sample);
};

class ForwardingCapturedFrameTap
    : public rtc::VideoSinkInterface<VideoFrame>,
      public rtc::VideoSourceInterface<VideoFrame> {
 public:
  ForwardingCapturedFrameTap(Clock* clock,
                             VideoQualityAnalyzer* analyzer,
                             rtc::VideoSourceInterface<VideoFrame>* source);
  ForwardingCapturedFrameTap(ForwardingCapturedFrameTap&) = delete;
  ForwardingCapturedFrameTap& operator=(ForwardingCapturedFrameTap&) = delete;
  ~ForwardingCapturedFrameTap();

  // VideoSinkInterface interface
  void OnFrame(const VideoFrame& frame) override;
  void OnDiscardedFrame() override;

  // VideoSourceInterface interface
  void AddOrUpdateSink(VideoSinkInterface<VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(VideoSinkInterface<VideoFrame>* sink) override;
  VideoFrame PopFrame();

 private:
  Clock* const clock_;
  VideoQualityAnalyzer* const analyzer_;
  rtc::VideoSourceInterface<VideoFrame>* const source_;
  VideoSinkInterface<VideoFrame>* sink_;
  int discarded_count_ = 0;
};

class DecodedFrameTap : public rtc::VideoSinkInterface<VideoFrame> {
 public:
  explicit DecodedFrameTap(VideoQualityAnalyzer* analyzer);
  // VideoSinkInterface interface
  void OnFrame(const VideoFrame& frame) override;

 private:
  VideoQualityAnalyzer* const analyzer_;
};
}  // namespace test
}  // namespace webrtc
#endif  // TEST_SCENARIO_QUALITY_STATS_H_
